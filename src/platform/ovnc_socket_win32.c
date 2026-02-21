#include "ovnc_platform.h"

#ifdef _WIN32

static int platform_init_count = 0;

ovnc_error_t ovnc__platform_init(void)
{
    if (platform_init_count == 0) {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            return OVNC_ERR_IO;
    }
    platform_init_count++;
    return OVNC_OK;
}

void ovnc__platform_cleanup(void)
{
    if (platform_init_count > 0) {
        platform_init_count--;
        if (platform_init_count == 0)
            WSACleanup();
    }
}

/*
 * Win32 socket operations are not yet implemented.
 * Full implementation is planned for a future phase.
 */

ovnc_error_t ovnc__socket_connect(const char *host, uint16_t port,
                                  uint32_t timeout_ms, ovnc_socket_t *out)
{
    (void)host;
    (void)port;
    (void)timeout_ms;
    *out = OVNC_INVALID_SOCKET;
    return OVNC_ERR_IO; /* Not yet implemented */
}

void ovnc__socket_close(ovnc_socket_t sock)
{
    if (sock != OVNC_INVALID_SOCKET)
        closesocket(sock);
}

ovnc_error_t ovnc__socket_send_all(ovnc_socket_t sock,
                                   const void *data, size_t len)
{
    (void)sock;
    (void)data;
    (void)len;
    return OVNC_ERR_IO; /* Not yet implemented */
}

ovnc_error_t ovnc__socket_recv_all(ovnc_socket_t sock,
                                   void *buf, size_t len)
{
    (void)sock;
    (void)buf;
    (void)len;
    return OVNC_ERR_IO; /* Not yet implemented */
}

#endif /* _WIN32 */
