#include "ovnc_internal.h"

/* Server API stubs - implementation deferred to Phase 6 */

ovnc_server_t* ovnc_server_create(const ovnc_server_config_t *config,
                                  const ovnc_server_callbacks_t *callbacks)
{
    (void)config;
    (void)callbacks;
    return NULL;
}

void ovnc_server_destroy(ovnc_server_t *server)
{
    (void)server;
}

ovnc_error_t ovnc_server_start(ovnc_server_t *server)
{
    (void)server;
    return OVNC_ERR_INVALID_ARG;
}

ovnc_error_t ovnc_server_stop(ovnc_server_t *server)
{
    (void)server;
    return OVNC_ERR_INVALID_ARG;
}

ovnc_error_t ovnc_server_run(ovnc_server_t *server)
{
    (void)server;
    return OVNC_ERR_INVALID_ARG;
}

ovnc_framebuffer_t* ovnc_server_get_framebuffer(ovnc_server_t *server)
{
    (void)server;
    return NULL;
}

ovnc_error_t ovnc_server_mark_rect_modified(ovnc_server_t *server,
                                            const ovnc_rect_t *rect)
{
    (void)server;
    (void)rect;
    return OVNC_ERR_INVALID_ARG;
}

ovnc_error_t ovnc_server_resize(ovnc_server_t *server,
                                uint16_t width, uint16_t height)
{
    (void)server;
    (void)width;
    (void)height;
    return OVNC_ERR_INVALID_ARG;
}

ovnc_error_t ovnc_server_send_bell(ovnc_server_t *server)
{
    (void)server;
    return OVNC_ERR_INVALID_ARG;
}

ovnc_error_t ovnc_server_send_cut_text(ovnc_server_t *server,
                                       const char *text, uint32_t length)
{
    (void)server;
    (void)text;
    (void)length;
    return OVNC_ERR_INVALID_ARG;
}

void* ovnc_server_get_user_data(const ovnc_server_t *server)
{
    (void)server;
    return NULL;
}

void ovnc_server_set_user_data(ovnc_server_t *server, void *data)
{
    (void)server;
    (void)data;
}
