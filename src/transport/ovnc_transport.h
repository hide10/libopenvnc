#ifndef OVNC_TRANSPORT_H
#define OVNC_TRANSPORT_H

#include <ovnc/ovnc_common.h>
#include "platform/ovnc_platform.h"

typedef struct ovnc_transport ovnc_transport_t;

struct ovnc_transport {
    ovnc_socket_t sock;
};

ovnc_error_t ovnc__transport_create(ovnc_transport_t **out);
void         ovnc__transport_destroy(ovnc_transport_t *t);
ovnc_error_t ovnc__transport_connect(ovnc_transport_t *t,
                                     const char *host, uint16_t port,
                                     uint32_t timeout_ms);
void         ovnc__transport_close(ovnc_transport_t *t);
ovnc_error_t ovnc__transport_send(ovnc_transport_t *t,
                                  const void *data, size_t len);
ovnc_error_t ovnc__transport_recv(ovnc_transport_t *t,
                                  void *buf, size_t len);

#endif /* OVNC_TRANSPORT_H */
