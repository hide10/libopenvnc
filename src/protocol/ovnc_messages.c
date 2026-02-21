#include "ovnc_messages.h"
#include "protocol/ovnc_encoding.h"
#include <stdlib.h>
#include <string.h>

/*-------------------------------------------------------------------
 * Client -> Server: SetPixelFormat (type 0, 20 bytes)
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__send_set_pixel_format(ovnc_client_t *client,
                                         const ovnc_pixel_format_t *fmt)
{
    uint8_t buf[20];
    memset(buf, 0, sizeof(buf));
    buf[0] = 0; /* message-type */
    /* buf[1..3] = padding */
    ovnc__pixel_format_to_wire(fmt, &buf[4]);
    return ovnc__transport_send(client->transport, buf, sizeof(buf));
}

/*-------------------------------------------------------------------
 * Client -> Server: SetEncodings (type 2, 4 + 4*n bytes)
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__send_set_encodings(ovnc_client_t *client,
                                      const ovnc_encoding_type_t *encodings,
                                      size_t num_encodings)
{
    size_t msg_len = 4 + 4 * num_encodings;
    uint8_t *buf = malloc(msg_len);
    if (!buf)
        return OVNC_ERR_NOMEM;

    memset(buf, 0, 4);
    buf[0] = 2; /* message-type */
    /* buf[1] = padding */
    ovnc__write_u16(&buf[2], (uint16_t)num_encodings);

    for (size_t i = 0; i < num_encodings; i++) {
        ovnc__write_s32(&buf[4 + i * 4], (int32_t)encodings[i]);
    }

    ovnc_error_t err = ovnc__transport_send(client->transport, buf, msg_len);
    free(buf);
    return err;
}

/*-------------------------------------------------------------------
 * Client -> Server: FramebufferUpdateRequest (type 3, 10 bytes)
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__send_fb_update_request(ovnc_client_t *client,
                                          uint8_t incremental,
                                          uint16_t x, uint16_t y,
                                          uint16_t width, uint16_t height)
{
    uint8_t buf[10];
    buf[0] = 3; /* message-type */
    buf[1] = incremental;
    ovnc__write_u16(&buf[2], x);
    ovnc__write_u16(&buf[4], y);
    ovnc__write_u16(&buf[6], width);
    ovnc__write_u16(&buf[8], height);
    return ovnc__transport_send(client->transport, buf, sizeof(buf));
}

/*-------------------------------------------------------------------
 * Client -> Server: KeyEvent (type 4, 8 bytes)
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__send_key_event(ovnc_client_t *client,
                                  uint32_t key, uint8_t down)
{
    uint8_t buf[8];
    buf[0] = 4; /* message-type */
    buf[1] = down;
    buf[2] = 0; /* padding */
    buf[3] = 0;
    ovnc__write_u32(&buf[4], key);
    return ovnc__transport_send(client->transport, buf, sizeof(buf));
}

/*-------------------------------------------------------------------
 * Client -> Server: PointerEvent (type 5, 6 bytes)
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__send_pointer_event(ovnc_client_t *client,
                                      uint16_t x, uint16_t y,
                                      uint8_t button_mask)
{
    uint8_t buf[6];
    buf[0] = 5; /* message-type */
    buf[1] = button_mask;
    ovnc__write_u16(&buf[2], x);
    ovnc__write_u16(&buf[4], y);
    return ovnc__transport_send(client->transport, buf, sizeof(buf));
}

/*-------------------------------------------------------------------
 * Client -> Server: ClientCutText (type 6, 8 + length bytes)
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__send_client_cut_text(ovnc_client_t *client,
                                        const char *text, uint32_t length)
{
    uint8_t header[8];
    memset(header, 0, sizeof(header));
    header[0] = 6; /* message-type */
    /* header[1..3] = padding */
    ovnc__write_u32(&header[4], length);

    ovnc_error_t err = ovnc__transport_send(client->transport,
                                            header, sizeof(header));
    if (err != OVNC_OK)
        return err;

    if (length > 0) {
        err = ovnc__transport_send(client->transport, text, length);
    }
    return err;
}

/*-------------------------------------------------------------------
 * Server -> Client: FramebufferUpdate (type 0)
 *-------------------------------------------------------------------*/

static ovnc_error_t handle_fb_update(ovnc_client_t *client)
{
    uint8_t header[3]; /* padding(1) + number-of-rectangles(2) */
    ovnc_error_t err = ovnc__transport_recv(client->transport, header, 3);
    if (err != OVNC_OK)
        return err;

    uint16_t num_rects = ovnc__read_u16(&header[1]);

    for (uint16_t i = 0; i < num_rects; i++) {
        uint8_t rect_header[12];
        err = ovnc__transport_recv(client->transport, rect_header, 12);
        if (err != OVNC_OK)
            return err;

        ovnc_rect_t rect;
        rect.x      = ovnc__read_u16(&rect_header[0]);
        rect.y      = ovnc__read_u16(&rect_header[2]);
        rect.width  = ovnc__read_u16(&rect_header[4]);
        rect.height = ovnc__read_u16(&rect_header[6]);
        int32_t encoding = ovnc__read_s32(&rect_header[8]);

        err = ovnc__decode_rect(client, &rect, encoding);
        if (err != OVNC_OK)
            return err;
    }

    if (client->callbacks.framebuffer_update_finished)
        client->callbacks.framebuffer_update_finished(client);

    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Server -> Client: SetColorMapEntries (type 1)
 *-------------------------------------------------------------------*/

static ovnc_error_t handle_color_map_entries(ovnc_client_t *client)
{
    uint8_t header[5]; /* padding(1) + first-color(2) + num-colors(2) */
    ovnc_error_t err = ovnc__transport_recv(client->transport, header, 5);
    if (err != OVNC_OK)
        return err;

    uint16_t first_color = ovnc__read_u16(&header[1]);
    uint16_t num_colors  = ovnc__read_u16(&header[3]);

    /* Read RGB entries: 6 bytes per color */
    size_t data_len = (size_t)num_colors * 6;
    uint8_t *data = malloc(data_len);
    if (!data)
        return OVNC_ERR_NOMEM;

    err = ovnc__transport_recv(client->transport, data, data_len);
    if (err != OVNC_OK) {
        free(data);
        return err;
    }

    if (client->callbacks.color_map_update) {
        uint16_t *red   = malloc(sizeof(uint16_t) * num_colors);
        uint16_t *green = malloc(sizeof(uint16_t) * num_colors);
        uint16_t *blue  = malloc(sizeof(uint16_t) * num_colors);
        if (!red || !green || !blue) {
            free(red);
            free(green);
            free(blue);
            free(data);
            return OVNC_ERR_NOMEM;
        }
        for (uint16_t i = 0; i < num_colors; i++) {
            red[i]   = ovnc__read_u16(&data[i * 6]);
            green[i] = ovnc__read_u16(&data[i * 6 + 2]);
            blue[i]  = ovnc__read_u16(&data[i * 6 + 4]);
        }
        client->callbacks.color_map_update(client, first_color,
                                           num_colors, red, green, blue);
        free(red);
        free(green);
        free(blue);
    }

    free(data);
    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Server -> Client: Bell (type 2)
 *-------------------------------------------------------------------*/

static ovnc_error_t handle_bell(ovnc_client_t *client)
{
    /* No additional data */
    if (client->callbacks.bell)
        client->callbacks.bell(client);
    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Server -> Client: ServerCutText (type 3)
 *-------------------------------------------------------------------*/

static ovnc_error_t handle_server_cut_text(ovnc_client_t *client)
{
    uint8_t header[7]; /* padding(3) + length(4) */
    ovnc_error_t err = ovnc__transport_recv(client->transport, header, 7);
    if (err != OVNC_OK)
        return err;

    uint32_t length = ovnc__read_u32(&header[3]);

    char *text = NULL;
    if (length > 0) {
        text = malloc(length + 1);
        if (!text)
            return OVNC_ERR_NOMEM;
        err = ovnc__transport_recv(client->transport, text, length);
        if (err != OVNC_OK) {
            free(text);
            return err;
        }
        text[length] = '\0';
    }

    if (client->callbacks.server_cut_text)
        client->callbacks.server_cut_text(client, text ? text : "", length);

    free(text);
    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Server message dispatcher
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__recv_server_message(ovnc_client_t *client)
{
    uint8_t msg_type;
    ovnc_error_t err = ovnc__transport_recv(client->transport, &msg_type, 1);
    if (err != OVNC_OK)
        return err;

    switch (msg_type) {
    case 0: return handle_fb_update(client);
    case 1: return handle_color_map_entries(client);
    case 2: return handle_bell(client);
    case 3: return handle_server_cut_text(client);
    default:
        return OVNC_ERR_PROTOCOL;
    }
}
