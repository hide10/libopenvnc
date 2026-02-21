#include "ovnc_internal.h"
#include "platform/ovnc_platform.h"
#include "transport/ovnc_transport.h"
#include "protocol/ovnc_handshake.h"
#include "protocol/ovnc_messages.h"
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------
 * Lifecycle
 *-------------------------------------------------------------------*/

ovnc_client_t* ovnc_client_create(const ovnc_client_config_t *config,
                                  const ovnc_client_callbacks_t *callbacks)
{
    if (!config || !config->host)
        return NULL;

    ovnc_client_t *c = calloc(1, sizeof(*c));
    if (!c)
        return NULL;

    /* Deep copy host */
    size_t host_len = strlen(config->host);
    c->host = malloc(host_len + 1);
    if (!c->host) {
        free(c);
        return NULL;
    }
    memcpy(c->host, config->host, host_len + 1);

    c->port    = config->port ? config->port : 5900;
    c->shared  = config->shared;
    c->connect_timeout_ms = config->connect_timeout_ms;
    c->user_data = config->user_data;

    /* Deep copy pixel_format */
    if (config->pixel_format) {
        c->pixel_format = malloc(sizeof(ovnc_pixel_format_t));
        if (!c->pixel_format) {
            free(c->host);
            free(c);
            return NULL;
        }
        *c->pixel_format = *config->pixel_format;
    }

    /* Deep copy encodings */
    if (config->encodings && config->num_encodings > 0) {
        size_t enc_size = config->num_encodings * sizeof(ovnc_encoding_type_t);
        c->encodings = malloc(enc_size);
        if (!c->encodings) {
            free(c->pixel_format);
            free(c->host);
            free(c);
            return NULL;
        }
        memcpy(c->encodings, config->encodings, enc_size);
        c->num_encodings = config->num_encodings;
    }

    /* Copy callbacks */
    if (callbacks)
        c->callbacks = *callbacks;

    c->state = OVNC_CLIENT_STATE_CREATED;
    return c;
}

void ovnc_client_destroy(ovnc_client_t *client)
{
    if (!client)
        return;

    if (client->state == OVNC_CLIENT_STATE_CONNECTED)
        ovnc_client_disconnect(client);

    /* Free framebuffer if we allocated it (no alloc_framebuffer callback) */
    if (!client->callbacks.alloc_framebuffer)
        free(client->framebuffer.data);

    free(client->encodings);
    free(client->pixel_format);
    free(client->host);
    free(client);
}

/*-------------------------------------------------------------------
 * Connection
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_connect(ovnc_client_t *client)
{
    if (!client)
        return OVNC_ERR_INVALID_ARG;
    if (client->state != OVNC_CLIENT_STATE_CREATED)
        return OVNC_ERR_INVALID_ARG;

    ovnc_error_t err;

    err = ovnc__platform_init();
    if (err != OVNC_OK)
        return err;

    err = ovnc__transport_create(&client->transport);
    if (err != OVNC_OK) {
        ovnc__platform_cleanup();
        return err;
    }

    err = ovnc__transport_connect(client->transport, client->host,
                                  client->port, client->connect_timeout_ms);
    if (err != OVNC_OK) {
        ovnc__transport_destroy(client->transport);
        client->transport = NULL;
        ovnc__platform_cleanup();
        return err;
    }

    err = ovnc__handshake(client);
    if (err != OVNC_OK) {
        ovnc__transport_destroy(client->transport);
        client->transport = NULL;
        ovnc__platform_cleanup();
        return err;
    }

    client->state = OVNC_CLIENT_STATE_CONNECTED;
    return OVNC_OK;
}

ovnc_error_t ovnc_client_disconnect(ovnc_client_t *client)
{
    if (!client)
        return OVNC_ERR_INVALID_ARG;
    if (client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_OK;

    ovnc__transport_destroy(client->transport);
    client->transport = NULL;
    client->state = OVNC_CLIENT_STATE_DISCONNECTED;
    ovnc__platform_cleanup();
    return OVNC_OK;
}

int ovnc_client_is_connected(const ovnc_client_t *client)
{
    return client && client->state == OVNC_CLIENT_STATE_CONNECTED;
}

ovnc_error_t ovnc_client_get_connection_info(const ovnc_client_t *client,
                                             ovnc_connection_info_t *info)
{
    if (!client || !info)
        return OVNC_ERR_INVALID_ARG;
    if (client->state == OVNC_CLIENT_STATE_CREATED)
        return OVNC_ERR_INVALID_ARG;
    *info = client->conn_info;
    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Message processing
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_process_message(ovnc_client_t *client)
{
    if (!client || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;

    ovnc_error_t err = ovnc__recv_server_message(client);
    if (err != OVNC_OK && err != OVNC_ERR_CONNECTION_CLOSED) {
        if (client->callbacks.disconnected)
            client->callbacks.disconnected(client, err);
        ovnc_client_disconnect(client);
    } else if (err == OVNC_ERR_CONNECTION_CLOSED) {
        if (client->callbacks.disconnected)
            client->callbacks.disconnected(client, err);
        ovnc_client_disconnect(client);
    }
    return err;
}

ovnc_error_t ovnc_client_run(ovnc_client_t *client)
{
    if (!client || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;

    ovnc_error_t err;
    while (client->state == OVNC_CLIENT_STATE_CONNECTED) {
        err = ovnc__recv_server_message(client);
        if (err != OVNC_OK) {
            if (client->callbacks.disconnected)
                client->callbacks.disconnected(client, err);
            ovnc_client_disconnect(client);
            return err;
        }
    }
    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Framebuffer update request
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_request_update(ovnc_client_t *client,
                                        const ovnc_rect_t *rect,
                                        int incremental)
{
    if (!client || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;

    uint16_t x, y, w, h;
    if (rect) {
        x = rect->x;
        y = rect->y;
        w = rect->width;
        h = rect->height;
    } else {
        x = 0;
        y = 0;
        w = client->framebuffer.width;
        h = client->framebuffer.height;
    }
    return ovnc__send_fb_update_request(client, (uint8_t)incremental,
                                        x, y, w, h);
}

/*-------------------------------------------------------------------
 * Input events
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_send_key_event(ovnc_client_t *client,
                                        uint32_t key, int down)
{
    if (!client || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;
    return ovnc__send_key_event(client, key, (uint8_t)(down ? 1 : 0));
}

ovnc_error_t ovnc_client_send_pointer_event(ovnc_client_t *client,
                                            uint16_t x, uint16_t y,
                                            uint8_t button_mask)
{
    if (!client || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;
    return ovnc__send_pointer_event(client, x, y, button_mask);
}

ovnc_error_t ovnc_client_send_cut_text(ovnc_client_t *client,
                                       const char *text, uint32_t length)
{
    if (!client || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;
    if (!text)
        return OVNC_ERR_INVALID_ARG;
    return ovnc__send_client_cut_text(client, text, length);
}

/*-------------------------------------------------------------------
 * Pixel format / encodings
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_set_pixel_format(ovnc_client_t *client,
                                          const ovnc_pixel_format_t *format)
{
    if (!client || !format || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;

    int new_bpp = ovnc_pixel_format_bytes_per_pixel(format);
    if (new_bpp < 0)
        return OVNC_ERR_INVALID_ARG;

    ovnc_error_t err = ovnc__send_set_pixel_format(client, format);
    if (err != OVNC_OK)
        return err;

    /* Update local format and reallocate framebuffer if bpp changed */
    int old_bpp = ovnc_pixel_format_bytes_per_pixel(&client->framebuffer.format);
    client->framebuffer.format = *format;

    size_t new_size = (size_t)client->framebuffer.width *
                      (size_t)client->framebuffer.height * (size_t)new_bpp;

    if (new_bpp != old_bpp) {
        client->framebuffer.data_size = new_size;
        if (client->callbacks.alloc_framebuffer) {
            err = client->callbacks.alloc_framebuffer(client,
                                                      &client->framebuffer);
            if (err != OVNC_OK)
                return err;
        } else {
            uint8_t *new_data = calloc(1, new_size);
            if (!new_data)
                return OVNC_ERR_NOMEM;
            free(client->framebuffer.data);
            client->framebuffer.data = new_data;
        }
    }

    return OVNC_OK;
}

ovnc_error_t ovnc_client_set_encodings(ovnc_client_t *client,
                                       const ovnc_encoding_type_t *encodings,
                                       size_t num_encodings)
{
    if (!client || !encodings || client->state != OVNC_CLIENT_STATE_CONNECTED)
        return OVNC_ERR_INVALID_ARG;
    return ovnc__send_set_encodings(client, encodings, num_encodings);
}

/*-------------------------------------------------------------------
 * Framebuffer access
 *-------------------------------------------------------------------*/

const ovnc_framebuffer_t* ovnc_client_get_framebuffer(const ovnc_client_t *client)
{
    if (!client)
        return NULL;
    return &client->framebuffer;
}

/*-------------------------------------------------------------------
 * User data
 *-------------------------------------------------------------------*/

void* ovnc_client_get_user_data(const ovnc_client_t *client)
{
    return client ? client->user_data : NULL;
}

void ovnc_client_set_user_data(ovnc_client_t *client, void *data)
{
    if (client)
        client->user_data = data;
}
