#ifndef OVNC_INTERNAL_H
#define OVNC_INTERNAL_H

#include <ovnc/ovnc_common.h>
#include <ovnc/ovnc_client.h>
#include <ovnc/ovnc_server.h>
#include <string.h>

/*-------------------------------------------------------------------
 * Byte order helpers (big-endian wire format)
 *-------------------------------------------------------------------*/

static inline uint16_t ovnc__read_u16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] << 8 | (uint16_t)p[1]);
}

static inline uint32_t ovnc__read_u32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 |
           (uint32_t)p[2] << 8  | (uint32_t)p[3];
}

static inline int32_t ovnc__read_s32(const uint8_t *p)
{
    uint32_t v = ovnc__read_u32(p);
    int32_t result;
    memcpy(&result, &v, sizeof(result));
    return result;
}

static inline void ovnc__write_u8(uint8_t *p, uint8_t val)
{
    p[0] = val;
}

static inline void ovnc__write_u16(uint8_t *p, uint16_t val)
{
    p[0] = (uint8_t)(val >> 8);
    p[1] = (uint8_t)(val);
}

static inline void ovnc__write_u32(uint8_t *p, uint32_t val)
{
    p[0] = (uint8_t)(val >> 24);
    p[1] = (uint8_t)(val >> 16);
    p[2] = (uint8_t)(val >> 8);
    p[3] = (uint8_t)(val);
}

static inline void ovnc__write_s32(uint8_t *p, int32_t val)
{
    uint32_t v;
    memcpy(&v, &val, sizeof(v));
    ovnc__write_u32(p, v);
}

/*-------------------------------------------------------------------
 * Transport (forward declaration)
 *-------------------------------------------------------------------*/

#ifndef OVNC_TRANSPORT_H
typedef struct ovnc_transport ovnc_transport_t;
#endif

/*-------------------------------------------------------------------
 * Client internal structure
 *-------------------------------------------------------------------*/

typedef enum {
    OVNC_CLIENT_STATE_CREATED,
    OVNC_CLIENT_STATE_CONNECTED,
    OVNC_CLIENT_STATE_DISCONNECTED,
} ovnc_client_state_t;

struct ovnc_client {
    /* Configuration (deep copies) */
    char                   *host;
    uint16_t                port;
    uint8_t                 shared;
    ovnc_pixel_format_t    *pixel_format;
    ovnc_encoding_type_t   *encodings;
    size_t                  num_encodings;
    uint32_t                connect_timeout_ms;
    void                   *user_data;

    /* Callbacks */
    ovnc_client_callbacks_t callbacks;

    /* State */
    ovnc_client_state_t     state;
    ovnc_transport_t       *transport;
    ovnc_connection_info_t  conn_info;
    ovnc_framebuffer_t      framebuffer;
};

/*-------------------------------------------------------------------
 * Pixel format wire serialization
 *-------------------------------------------------------------------*/

void ovnc__pixel_format_to_wire(const ovnc_pixel_format_t *fmt, uint8_t buf[16]);
void ovnc__pixel_format_from_wire(const uint8_t buf[16], ovnc_pixel_format_t *fmt);

#endif /* OVNC_INTERNAL_H */
