#ifndef OVNC_COMMON_H
#define OVNC_COMMON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------
 * Error codes
 *-------------------------------------------------------------------*/

typedef enum {
    OVNC_OK = 0,
    OVNC_ERR_NOMEM,
    OVNC_ERR_INVALID_ARG,
    OVNC_ERR_IO,
    OVNC_ERR_TIMEOUT,
    OVNC_ERR_CONNECTION_REFUSED,
    OVNC_ERR_CONNECTION_CLOSED,
    OVNC_ERR_VERSION_MISMATCH,
    OVNC_ERR_AUTH_FAILED,
    OVNC_ERR_AUTH_UNSUPPORTED,
    OVNC_ERR_PROTOCOL,
    OVNC_ERR_ENCODING_UNSUPPORTED,
    OVNC_ERR_ZLIB,
} ovnc_error_t;

const char* ovnc_error_string(ovnc_error_t error);

/*-------------------------------------------------------------------
 * RFB version
 *-------------------------------------------------------------------*/

typedef enum {
    OVNC_RFB_VERSION_33 = 33,
    OVNC_RFB_VERSION_37 = 37,
    OVNC_RFB_VERSION_38 = 38,
} ovnc_rfb_version_t;

/*-------------------------------------------------------------------
 * Security types
 *-------------------------------------------------------------------*/

typedef enum {
    OVNC_SECURITY_INVALID  = 0,
    OVNC_SECURITY_NONE     = 1,
    OVNC_SECURITY_VNC_AUTH = 2,
} ovnc_security_type_t;

/*-------------------------------------------------------------------
 * Encoding types
 *-------------------------------------------------------------------*/

typedef enum {
    OVNC_ENCODING_RAW          =  0,
    OVNC_ENCODING_COPYRECT     =  1,
    OVNC_ENCODING_RRE          =  2,
    OVNC_ENCODING_HEXTILE      =  5,
    OVNC_ENCODING_TRLE         = 15,
    OVNC_ENCODING_ZRLE         = 16,
    OVNC_ENCODING_CURSOR       = -239,
    OVNC_ENCODING_DESKTOP_SIZE = -223,
} ovnc_encoding_type_t;

/*-------------------------------------------------------------------
 * Pixel format
 *-------------------------------------------------------------------*/

typedef struct {
    uint8_t  bits_per_pixel;
    uint8_t  depth;
    uint8_t  big_endian;
    uint8_t  true_color;
    uint16_t red_max;
    uint16_t green_max;
    uint16_t blue_max;
    uint8_t  red_shift;
    uint8_t  green_shift;
    uint8_t  blue_shift;
} ovnc_pixel_format_t;

void ovnc_pixel_format_init_default(ovnc_pixel_format_t *fmt);
int  ovnc_pixel_format_bytes_per_pixel(const ovnc_pixel_format_t *fmt);

/*-------------------------------------------------------------------
 * Framebuffer
 *-------------------------------------------------------------------*/

typedef struct {
    uint16_t             width;
    uint16_t             height;
    ovnc_pixel_format_t  format;
    uint8_t             *data;
    size_t               data_size;
} ovnc_framebuffer_t;

/*-------------------------------------------------------------------
 * Rectangle
 *-------------------------------------------------------------------*/

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ovnc_rect_t;

/*-------------------------------------------------------------------
 * Forward declarations
 *-------------------------------------------------------------------*/

typedef struct ovnc_client ovnc_client_t;
typedef struct ovnc_server ovnc_server_t;
typedef struct ovnc_server_client ovnc_server_client_t;

/*-------------------------------------------------------------------
 * Connection info
 *-------------------------------------------------------------------*/

#define OVNC_SERVER_NAME_MAX 256

typedef struct {
    char                  name[OVNC_SERVER_NAME_MAX];
    uint32_t              name_length;
    int                   name_truncated;
    uint16_t              width;
    uint16_t              height;
    ovnc_pixel_format_t   pixel_format;
    ovnc_rfb_version_t    rfb_version;
    ovnc_security_type_t  security_type;
} ovnc_connection_info_t;

#ifdef __cplusplus
}
#endif

#endif /* OVNC_COMMON_H */
