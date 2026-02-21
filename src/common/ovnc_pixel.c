#include "ovnc_internal.h"

#define OVNC_CHANNEL_BITS  8
#define OVNC_CHANNEL_MAX   ((1 << OVNC_CHANNEL_BITS) - 1) /* 255 */

void ovnc_pixel_format_init_default(ovnc_pixel_format_t *fmt)
{
    /* 32bpp BGRA (common desktop format, 8 bits per channel) */
    fmt->bits_per_pixel = 32;
    fmt->depth          = 3 * OVNC_CHANNEL_BITS; /* 24 */
    fmt->big_endian     = 0;
    fmt->true_color     = 1;
    fmt->red_max        = OVNC_CHANNEL_MAX;
    fmt->green_max      = OVNC_CHANNEL_MAX;
    fmt->blue_max       = OVNC_CHANNEL_MAX;
    fmt->red_shift      = 2 * OVNC_CHANNEL_BITS; /* 16 */
    fmt->green_shift    = 1 * OVNC_CHANNEL_BITS; /* 8 */
    fmt->blue_shift     = 0;
}

int ovnc_pixel_format_bytes_per_pixel(const ovnc_pixel_format_t *fmt)
{
    switch (fmt->bits_per_pixel) {
    case 8:  return 1;
    case 16: return 2;
    case 32: return 4;
    default: return -1;
    }
}

void ovnc__pixel_format_to_wire(const ovnc_pixel_format_t *fmt, uint8_t buf[16])
{
    memset(buf, 0, 16);
    buf[0]  = fmt->bits_per_pixel;
    buf[1]  = fmt->depth;
    buf[2]  = fmt->big_endian;
    buf[3]  = fmt->true_color;
    ovnc__write_u16(&buf[4], fmt->red_max);
    ovnc__write_u16(&buf[6], fmt->green_max);
    ovnc__write_u16(&buf[8], fmt->blue_max);
    buf[10] = fmt->red_shift;
    buf[11] = fmt->green_shift;
    buf[12] = fmt->blue_shift;
    /* buf[13..15] = padding (already zero) */
}

void ovnc__pixel_format_from_wire(const uint8_t buf[16], ovnc_pixel_format_t *fmt)
{
    fmt->bits_per_pixel = buf[0];
    fmt->depth          = buf[1];
    fmt->big_endian     = buf[2];
    fmt->true_color     = buf[3];
    fmt->red_max        = ovnc__read_u16(&buf[4]);
    fmt->green_max      = ovnc__read_u16(&buf[6]);
    fmt->blue_max       = ovnc__read_u16(&buf[8]);
    fmt->red_shift      = buf[10];
    fmt->green_shift    = buf[11];
    fmt->blue_shift     = buf[12];
}
