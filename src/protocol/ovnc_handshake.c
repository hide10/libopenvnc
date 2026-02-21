#include "ovnc_handshake.h"
#include "protocol/ovnc_messages.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define RFB_VERSION_LEN 12

/*-------------------------------------------------------------------
 * Version negotiation
 *-------------------------------------------------------------------*/

static ovnc_error_t negotiate_version(ovnc_client_t *client)
{
    uint8_t buf[RFB_VERSION_LEN];
    ovnc_error_t err;

    /* Receive server version */
    err = ovnc__transport_recv(client->transport, buf, RFB_VERSION_LEN);
    if (err != OVNC_OK)
        return err;

    /* Parse "RFB xxx.yyy\n" */
    if (memcmp(buf, "RFB ", 4) != 0 || buf[7] != '.' || buf[11] != '\n')
        return OVNC_ERR_PROTOCOL;

    int major = (buf[4] - '0') * 100 + (buf[5] - '0') * 10 + (buf[6] - '0');
    int minor = (buf[8] - '0') * 100 + (buf[9] - '0') * 10 + (buf[10] - '0');

    /* Determine version to use */
    ovnc_rfb_version_t version;
    if (major != 3) {
        return OVNC_ERR_VERSION_MISMATCH;
    }
    if (minor >= 8) {
        version = OVNC_RFB_VERSION_38;
    } else if (minor == 7) {
        version = OVNC_RFB_VERSION_37;
    } else {
        version = OVNC_RFB_VERSION_33;
    }

    client->conn_info.rfb_version = version;

    /* Send our version */
    const char *version_str;
    switch (version) {
    case OVNC_RFB_VERSION_38: version_str = "RFB 003.008\n"; break;
    case OVNC_RFB_VERSION_37: version_str = "RFB 003.007\n"; break;
    default:                  version_str = "RFB 003.003\n"; break;
    }

    return ovnc__transport_send(client->transport, version_str, RFB_VERSION_LEN);
}

/*-------------------------------------------------------------------
 * Security handshake
 *-------------------------------------------------------------------*/

static ovnc_error_t read_failure_reason(ovnc_transport_t *t)
{
    uint8_t len_buf[4];
    ovnc_error_t err = ovnc__transport_recv(t, len_buf, 4);
    if (err != OVNC_OK)
        return err;

    uint32_t reason_len = ovnc__read_u32(len_buf);
    /* Skip the reason string */
    if (reason_len > 0) {
        uint8_t *tmp = malloc(reason_len);
        if (tmp) {
            ovnc__transport_recv(t, tmp, reason_len);
            free(tmp);
        }
    }
    return OVNC_ERR_CONNECTION_REFUSED;
}

static ovnc_error_t negotiate_security(ovnc_client_t *client)
{
    ovnc_error_t err;
    ovnc_rfb_version_t version = client->conn_info.rfb_version;
    ovnc_transport_t *t = client->transport;

    if (version == OVNC_RFB_VERSION_33) {
        /* RFB 3.3: server sends U32 security type */
        uint8_t buf[4];
        err = ovnc__transport_recv(t, buf, 4);
        if (err != OVNC_OK)
            return err;

        uint32_t sec_type = ovnc__read_u32(buf);
        if (sec_type == 0) {
            return read_failure_reason(t);
        }

        /* RFB 3.3: server chose the type, we can only accept None */
        if (sec_type != OVNC_SECURITY_NONE)
            return OVNC_ERR_AUTH_UNSUPPORTED;

        client->conn_info.security_type = (ovnc_security_type_t)sec_type;
    } else {
        /* RFB 3.7 / 3.8: server sends list, client chooses */
        uint8_t count;
        err = ovnc__transport_recv(t, &count, 1);
        if (err != OVNC_OK)
            return err;

        if (count == 0) {
            return read_failure_reason(t);
        }

        uint8_t *types = malloc(count);
        if (!types)
            return OVNC_ERR_NOMEM;

        err = ovnc__transport_recv(t, types, count);
        if (err != OVNC_OK) {
            free(types);
            return err;
        }

        /* Choose None (1) if available; we don't support VNC Auth yet */
        ovnc_security_type_t selected = OVNC_SECURITY_INVALID;
        for (uint8_t i = 0; i < count; i++) {
            if (types[i] == OVNC_SECURITY_NONE) {
                selected = OVNC_SECURITY_NONE;
                break;
            }
        }
        free(types);

        if (selected == OVNC_SECURITY_INVALID)
            return OVNC_ERR_AUTH_UNSUPPORTED;

        /* Send selected type */
        uint8_t sel = (uint8_t)selected;
        err = ovnc__transport_send(t, &sel, 1);
        if (err != OVNC_OK)
            return err;

        client->conn_info.security_type = selected;
    }

    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Security result
 *-------------------------------------------------------------------*/

static ovnc_error_t check_security_result(ovnc_client_t *client)
{
    ovnc_rfb_version_t version = client->conn_info.rfb_version;
    ovnc_security_type_t sec = client->conn_info.security_type;
    ovnc_transport_t *t = client->transport;

    if (version == OVNC_RFB_VERSION_38) {
        /* 3.8: always has SecurityResult */
        uint8_t buf[4];
        ovnc_error_t err = ovnc__transport_recv(t, buf, 4);
        if (err != OVNC_OK)
            return err;

        uint32_t status = ovnc__read_u32(buf);
        if (status != 0) {
            read_failure_reason(t);
            return OVNC_ERR_AUTH_FAILED;
        }
    } else if (sec == OVNC_SECURITY_VNC_AUTH) {
        /* 3.3/3.7: SecurityResult only for VNC Auth */
        uint8_t buf[4];
        ovnc_error_t err = ovnc__transport_recv(t, buf, 4);
        if (err != OVNC_OK)
            return err;

        uint32_t status = ovnc__read_u32(buf);
        if (status != 0)
            return OVNC_ERR_AUTH_FAILED;
    }
    /* 3.3/3.7 with None: no SecurityResult */

    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Initialization (ClientInit + ServerInit)
 *-------------------------------------------------------------------*/

static ovnc_error_t do_initialization(ovnc_client_t *client)
{
    ovnc_error_t err;
    ovnc_transport_t *t = client->transport;

    /* ClientInit: send shared-flag */
    uint8_t shared = client->shared;
    err = ovnc__transport_send(t, &shared, 1);
    if (err != OVNC_OK)
        return err;

    /* ServerInit: width(2) + height(2) + pixel_format(16) + name_length(4) */
    uint8_t header[24];
    err = ovnc__transport_recv(t, header, 24);
    if (err != OVNC_OK)
        return err;

    client->conn_info.width  = ovnc__read_u16(&header[0]);
    client->conn_info.height = ovnc__read_u16(&header[2]);
    ovnc__pixel_format_from_wire(&header[4], &client->conn_info.pixel_format);

    uint32_t name_len = ovnc__read_u32(&header[20]);
    client->conn_info.name_length = name_len;

    /* Read server name */
    if (name_len > 0) {
        uint32_t read_len = name_len;
        if (read_len > OVNC_SERVER_NAME_MAX - 1) {
            /* Read what we can store, then skip the rest */
            err = ovnc__transport_recv(t, client->conn_info.name,
                                       OVNC_SERVER_NAME_MAX - 1);
            if (err != OVNC_OK)
                return err;
            client->conn_info.name[OVNC_SERVER_NAME_MAX - 1] = '\0';
            client->conn_info.name_truncated = 1;

            /* Skip remaining bytes using fixed-size chunk reads */
            uint32_t skip = name_len - (OVNC_SERVER_NAME_MAX - 1);
            uint8_t discard[256];
            while (skip > 0) {
                uint32_t chunk = skip > sizeof(discard) ? sizeof(discard) : skip;
                err = ovnc__transport_recv(t, discard, chunk);
                if (err != OVNC_OK)
                    return err;
                skip -= chunk;
            }
        } else {
            err = ovnc__transport_recv(t, client->conn_info.name, read_len);
            if (err != OVNC_OK)
                return err;
            client->conn_info.name[read_len] = '\0';
            client->conn_info.name_truncated = 0;
        }
    } else {
        client->conn_info.name[0] = '\0';
        client->conn_info.name_truncated = 0;
    }

    /* Initialize framebuffer */
    ovnc_pixel_format_t active_fmt;
    if (client->pixel_format) {
        active_fmt = *client->pixel_format;
    } else {
        active_fmt = client->conn_info.pixel_format;
    }

    client->framebuffer.width  = client->conn_info.width;
    client->framebuffer.height = client->conn_info.height;
    client->framebuffer.format = active_fmt;

    int bpp = ovnc_pixel_format_bytes_per_pixel(&active_fmt);
    if (bpp < 0)
        return OVNC_ERR_PROTOCOL;
    size_t data_size = (size_t)client->framebuffer.width *
                       (size_t)client->framebuffer.height * (size_t)bpp;
    client->framebuffer.data_size = data_size;

    /* Call alloc_framebuffer callback or allocate internally */
    if (client->callbacks.alloc_framebuffer) {
        err = client->callbacks.alloc_framebuffer(client, &client->framebuffer);
        if (err != OVNC_OK)
            return err;
    } else {
        client->framebuffer.data = calloc(1, data_size);
        if (!client->framebuffer.data)
            return OVNC_ERR_NOMEM;
    }

    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Post-handshake setup
 *-------------------------------------------------------------------*/

static ovnc_error_t send_initial_messages(ovnc_client_t *client)
{
    ovnc_error_t err;

    /* Send SetPixelFormat if custom format requested */
    if (client->pixel_format) {
        err = ovnc__send_set_pixel_format(client, client->pixel_format);
        if (err != OVNC_OK)
            return err;
    }

    /* Send SetEncodings if specified */
    if (client->encodings && client->num_encodings > 0) {
        err = ovnc__send_set_encodings(client, client->encodings,
                                       client->num_encodings);
        if (err != OVNC_OK)
            return err;
    }

    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Public handshake entry point
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__handshake(ovnc_client_t *client)
{
    ovnc_error_t err;

    err = negotiate_version(client);
    if (err != OVNC_OK)
        return err;

    err = negotiate_security(client);
    if (err != OVNC_OK)
        return err;

    err = check_security_result(client);
    if (err != OVNC_OK)
        return err;

    err = do_initialization(client);
    if (err != OVNC_OK)
        return err;

    err = send_initial_messages(client);
    if (err != OVNC_OK)
        return err;

    return OVNC_OK;
}
