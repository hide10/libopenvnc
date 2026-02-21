#ifndef OVNC_PLATFORM_H
#define OVNC_PLATFORM_H

#include <ovnc/ovnc_common.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET ovnc_socket_t;
#define OVNC_INVALID_SOCKET INVALID_SOCKET
#else
typedef int ovnc_socket_t;
#define OVNC_INVALID_SOCKET (-1)
#endif

/*-------------------------------------------------------------------
 * Platform initialization
 *-------------------------------------------------------------------*/

ovnc_error_t ovnc__platform_init(void);
void         ovnc__platform_cleanup(void);

/*-------------------------------------------------------------------
 * Socket operations
 *-------------------------------------------------------------------*/

ovnc_error_t    ovnc__socket_connect(const char *host, uint16_t port,
                                     uint32_t timeout_ms, ovnc_socket_t *out);
void            ovnc__socket_close(ovnc_socket_t sock);
ovnc_error_t    ovnc__socket_send_all(ovnc_socket_t sock,
                                      const void *data, size_t len);
ovnc_error_t    ovnc__socket_recv_all(ovnc_socket_t sock,
                                      void *buf, size_t len);

#endif /* OVNC_PLATFORM_H */
