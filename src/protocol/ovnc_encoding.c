#include "ovnc_encoding.h"
#include "transport/ovnc_transport.h"
#include <stdlib.h>

/*-------------------------------------------------------------------
 * DesktopSize pseudo-encoding (-223): no pixel data
 *-------------------------------------------------------------------*/

static ovnc_error_t decode_desktop_size(ovnc_client_t *client,
                                        const ovnc_rect_t *rect)
{
    uint16_t new_width  = rect->width;
    uint16_t new_height = rect->height;

    /* Reallocate framebuffer */
    int bpp = ovnc_pixel_format_bytes_per_pixel(&client->framebuffer.format);
    size_t new_size = (size_t)new_width * (size_t)new_height * (size_t)bpp;

    if (client->callbacks.alloc_framebuffer) {
        client->framebuffer.width     = new_width;
        client->framebuffer.height    = new_height;
        client->framebuffer.data_size = new_size;
        ovnc_error_t err = client->callbacks.alloc_framebuffer(
            client, &client->framebuffer);
        if (err != OVNC_OK)
            return err;
    } else {
        uint8_t *new_data = calloc(1, new_size);
        if (!new_data)
            return OVNC_ERR_NOMEM;
        free(client->framebuffer.data);
        client->framebuffer.data      = new_data;
        client->framebuffer.width     = new_width;
        client->framebuffer.height    = new_height;
        client->framebuffer.data_size = new_size;
    }

    if (client->callbacks.desktop_resize)
        client->callbacks.desktop_resize(client, new_width, new_height);

    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Cursor pseudo-encoding (-239)
 *-------------------------------------------------------------------*/

static ovnc_error_t decode_cursor(ovnc_client_t *client,
                                  const ovnc_rect_t *rect)
{
    int bpp = ovnc_pixel_format_bytes_per_pixel(&client->framebuffer.format);
    size_t pixels_size = (size_t)rect->width * (size_t)rect->height * (size_t)bpp;
    size_t mask_row = ((size_t)rect->width + 7) / 8;
    size_t mask_size = mask_row * rect->height;
    size_t total = pixels_size + mask_size;

    if (total == 0) {
        if (client->callbacks.cursor_update)
            client->callbacks.cursor_update(client, rect->x, rect->y,
                                            0, 0, NULL, NULL);
        return OVNC_OK;
    }

    uint8_t *buf = malloc(total);
    if (!buf)
        return OVNC_ERR_NOMEM;

    ovnc_error_t err = ovnc__transport_recv(client->transport, buf, total);
    if (err != OVNC_OK) {
        free(buf);
        return err;
    }

    if (client->callbacks.cursor_update) {
        client->callbacks.cursor_update(client, rect->x, rect->y,
                                        rect->width, rect->height,
                                        buf, buf + pixels_size);
    }

    free(buf);
    return OVNC_OK;
}

/*-------------------------------------------------------------------
 * Encoding dispatcher
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__decode_rect(ovnc_client_t *client,
                               const ovnc_rect_t *rect,
                               int32_t encoding_type)
{
    switch (encoding_type) {
    case OVNC_ENCODING_RAW:
        return ovnc__decode_raw(client, rect);
    case OVNC_ENCODING_DESKTOP_SIZE:
        return decode_desktop_size(client, rect);
    case OVNC_ENCODING_CURSOR:
        return decode_cursor(client, rect);
    default:
        return OVNC_ERR_ENCODING_UNSUPPORTED;
    }
}
