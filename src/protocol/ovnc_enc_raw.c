#include "ovnc_encoding.h"
#include "transport/ovnc_transport.h"
#include <stdlib.h>

ovnc_error_t ovnc__decode_raw(ovnc_client_t *client,
                              const ovnc_rect_t *rect)
{
    int bpp = ovnc_pixel_format_bytes_per_pixel(&client->framebuffer.format);
    size_t row_bytes = (size_t)rect->width * (size_t)bpp;
    size_t fb_stride = (size_t)client->framebuffer.width * (size_t)bpp;

    /* Read row by row directly into framebuffer */
    for (uint16_t y = 0; y < rect->height; y++) {
        size_t offset = ((size_t)(rect->y + y) * fb_stride) +
                        ((size_t)rect->x * (size_t)bpp);

        if (offset + row_bytes > client->framebuffer.data_size)
            return OVNC_ERR_PROTOCOL;

        ovnc_error_t err = ovnc__transport_recv(
            client->transport,
            client->framebuffer.data + offset,
            row_bytes);
        if (err != OVNC_OK)
            return err;
    }

    if (client->callbacks.framebuffer_update)
        client->callbacks.framebuffer_update(client, rect);

    return OVNC_OK;
}
