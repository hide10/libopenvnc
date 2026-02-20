#include "ovnc_transport.h"
#include "ovnc_internal.h"
#include <stdlib.h>

ovnc_error_t ovnc__transport_create(ovnc_transport_t **out)
{
    ovnc_transport_t *t = calloc(1, sizeof(*t));
    if (!t)
        return OVNC_ERR_NOMEM;
    t->sock = OVNC_INVALID_SOCKET;
    *out = t;
    return OVNC_OK;
}

void ovnc__transport_destroy(ovnc_transport_t *t)
{
    if (!t)
        return;
    ovnc__transport_close(t);
    free(t);
}

ovnc_error_t ovnc__transport_connect(ovnc_transport_t *t,
                                     const char *host, uint16_t port,
                                     uint32_t timeout_ms)
{
    return ovnc__socket_connect(host, port, timeout_ms, &t->sock);
}

void ovnc__transport_close(ovnc_transport_t *t)
{
    if (t->sock != OVNC_INVALID_SOCKET) {
        ovnc__socket_close(t->sock);
        t->sock = OVNC_INVALID_SOCKET;
    }
}

ovnc_error_t ovnc__transport_send(ovnc_transport_t *t,
                                  const void *data, size_t len)
{
    return ovnc__socket_send_all(t->sock, data, len);
}

ovnc_error_t ovnc__transport_recv(ovnc_transport_t *t,
                                  void *buf, size_t len)
{
    return ovnc__socket_recv_all(t->sock, buf, len);
}
