#ifndef OVNC_SERVER_H
#define OVNC_SERVER_H

#include <ovnc/ovnc_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------------
 * Callback types
 *-------------------------------------------------------------------*/

typedef int (*ovnc_client_connected_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client
);

typedef void (*ovnc_client_disconnected_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client
);

typedef void (*ovnc_server_key_event_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    uint32_t key,
    int down
);

typedef void (*ovnc_server_pointer_event_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    uint16_t x,
    uint16_t y,
    uint8_t button_mask
);

typedef void (*ovnc_server_cut_text_event_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    const char *text,
    uint32_t length
);

typedef int (*ovnc_verify_password_fn)(
    ovnc_server_t *server,
    ovnc_server_client_t *client,
    const char *password
);

/*-------------------------------------------------------------------
 * Server callbacks
 *-------------------------------------------------------------------*/

typedef struct {
    ovnc_client_connected_fn     client_connected;
    ovnc_client_disconnected_fn  client_disconnected;
    ovnc_server_key_event_fn     key_event;
    ovnc_server_pointer_event_fn pointer_event;
    ovnc_server_cut_text_event_fn cut_text;
    ovnc_verify_password_fn      verify_password;
} ovnc_server_callbacks_t;

/*-------------------------------------------------------------------
 * Server configuration
 *-------------------------------------------------------------------*/

typedef struct {
    const char              *name;
    uint16_t                 port;
    uint16_t                 width;
    uint16_t                 height;
    ovnc_pixel_format_t      pixel_format;
    ovnc_security_type_t    *security_types;
    size_t                   num_security_types;
    void                    *user_data;
} ovnc_server_config_t;

/*-------------------------------------------------------------------
 * Lifecycle
 *-------------------------------------------------------------------*/

ovnc_server_t* ovnc_server_create(
    const ovnc_server_config_t *config,
    const ovnc_server_callbacks_t *callbacks
);

void ovnc_server_destroy(ovnc_server_t *server);

/*-------------------------------------------------------------------
 * Server control
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_server_start(ovnc_server_t *server);
ovnc_error_t ovnc_server_stop(ovnc_server_t *server);
ovnc_error_t ovnc_server_run(ovnc_server_t *server);

/*-------------------------------------------------------------------
 * Framebuffer operations
 *-------------------------------------------------------------------*/

ovnc_framebuffer_t* ovnc_server_get_framebuffer(ovnc_server_t *server);

ovnc_error_t ovnc_server_mark_rect_modified(
    ovnc_server_t *server,
    const ovnc_rect_t *rect
);

ovnc_error_t ovnc_server_resize(
    ovnc_server_t *server,
    uint16_t width,
    uint16_t height
);

/*-------------------------------------------------------------------
 * Client message sending
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc_server_send_bell(ovnc_server_t *server);

ovnc_error_t ovnc_server_send_cut_text(
    ovnc_server_t *server,
    const char *text,
    uint32_t length
);

/*-------------------------------------------------------------------
 * User data
 *-------------------------------------------------------------------*/

void* ovnc_server_get_user_data(const ovnc_server_t *server);
void  ovnc_server_set_user_data(ovnc_server_t *server, void *data);

#ifdef __cplusplus
}
#endif

#endif /* OVNC_SERVER_H */
