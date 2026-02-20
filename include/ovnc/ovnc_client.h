#ifndef OVNC_CLIENT_H
#define OVNC_CLIENT_H

#include <ovnc/ovnc_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------
 * Callback types
 *-------------------------------------------------------------------*/

typedef ovnc_error_t (*ovnc_alloc_framebuffer_fn)(
    ovnc_client_t *client,
    ovnc_framebuffer_t *fb
);

typedef void (*ovnc_framebuffer_update_fn)(
    ovnc_client_t *client,
    const ovnc_rect_t *rect
);

typedef void (*ovnc_framebuffer_update_finished_fn)(
    ovnc_client_t *client
);

typedef void (*ovnc_cursor_update_fn)(
    ovnc_client_t *client,
    uint16_t hotspot_x,
    uint16_t hotspot_y,
    uint16_t width,
    uint16_t height,
    const uint8_t *pixels,
    const uint8_t *bitmask
);

typedef void (*ovnc_desktop_resize_fn)(
    ovnc_client_t *client,
    uint16_t width,
    uint16_t height
);

typedef void (*ovnc_bell_fn)(ovnc_client_t *client);

typedef void (*ovnc_server_cut_text_fn)(
    ovnc_client_t *client,
    const char *text,
    uint32_t length
);

typedef void (*ovnc_color_map_update_fn)(
    ovnc_client_t *client,
    uint16_t first_color,
    uint16_t num_colors,
    const uint16_t *red,
    const uint16_t *green,
    const uint16_t *blue
);

typedef void (*ovnc_disconnected_fn)(
    ovnc_client_t *client,
    ovnc_error_t reason
);

typedef int (*ovnc_get_password_fn)(
    ovnc_client_t *client,
    char *buf,
    size_t buf_size
);

/*-------------------------------------------------------------------
 * Callback structure
 *-------------------------------------------------------------------*/

typedef struct {
    ovnc_alloc_framebuffer_fn           alloc_framebuffer;
    ovnc_framebuffer_update_fn          framebuffer_update;
    ovnc_framebuffer_update_finished_fn framebuffer_update_finished;
    ovnc_cursor_update_fn               cursor_update;
    ovnc_desktop_resize_fn              desktop_resize;
    ovnc_bell_fn                        bell;
    ovnc_server_cut_text_fn             server_cut_text;
    ovnc_color_map_update_fn            color_map_update;
    ovnc_disconnected_fn                disconnected;
    ovnc_get_password_fn                get_password;
} ovnc_client_callbacks_t;

/*-------------------------------------------------------------------
 * Client configuration
 *-------------------------------------------------------------------*/

typedef struct {
    const char                 *host;
    uint16_t                    port;
    uint8_t                     shared;
    ovnc_pixel_format_t        *pixel_format;
    const ovnc_encoding_type_t *encodings;
    size_t                      num_encodings;
    uint32_t                    connect_timeout_ms;
    void                       *user_data;
} ovnc_client_config_t;

/*-------------------------------------------------------------------
 * Lifecycle
 *-------------------------------------------------------------------*/

ovnc_client_t* ovnc_client_create(
    const ovnc_client_config_t *config,
    const ovnc_client_callbacks_t *callbacks
);

void ovnc_client_destroy(ovnc_client_t *client);

/*-------------------------------------------------------------------
 * Connection
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_connect(ovnc_client_t *client);
ovnc_error_t ovnc_client_disconnect(ovnc_client_t *client);
int          ovnc_client_is_connected(const ovnc_client_t *client);

ovnc_error_t ovnc_client_get_connection_info(
    const ovnc_client_t *client,
    ovnc_connection_info_t *info
);

/*-------------------------------------------------------------------
 * Message processing loop
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_process_message(ovnc_client_t *client);
ovnc_error_t ovnc_client_run(ovnc_client_t *client);

/*-------------------------------------------------------------------
 * Framebuffer update request
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_request_update(
    ovnc_client_t *client,
    const ovnc_rect_t *rect,
    int incremental
);

/*-------------------------------------------------------------------
 * Input events
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_send_key_event(
    ovnc_client_t *client,
    uint32_t key,
    int down
);

ovnc_error_t ovnc_client_send_pointer_event(
    ovnc_client_t *client,
    uint16_t x,
    uint16_t y,
    uint8_t button_mask
);

ovnc_error_t ovnc_client_send_cut_text(
    ovnc_client_t *client,
    const char *text,
    uint32_t length
);

/*-------------------------------------------------------------------
 * Pixel format / encodings
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_client_set_pixel_format(
    ovnc_client_t *client,
    const ovnc_pixel_format_t *format
);

ovnc_error_t ovnc_client_set_encodings(
    ovnc_client_t *client,
    const ovnc_encoding_type_t *encodings,
    size_t num_encodings
);

/*-------------------------------------------------------------------
 * Framebuffer access
 *-------------------------------------------------------------------*/

const ovnc_framebuffer_t* ovnc_client_get_framebuffer(
    const ovnc_client_t *client
);

/*-------------------------------------------------------------------
 * User data
 *-------------------------------------------------------------------*/

void* ovnc_client_get_user_data(const ovnc_client_t *client);
void  ovnc_client_set_user_data(ovnc_client_t *client, void *data);

#ifdef __cplusplus
}
#endif

#endif /* OVNC_CLIENT_H */
