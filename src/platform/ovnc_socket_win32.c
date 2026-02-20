#include "ovnc_platform.h"

#ifdef _WIN32

ovnc_error_t ovnc__platform_init(void)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return OVNC_ERR_IO;
    return OVNC_OK;
}

void ovnc__platform_cleanup(void)
{
    WSACleanup();
}

ovnc_error_t ovnc__socket_connect(const char *host, uint16_t port,
                                  uint32_t timeout_ms, ovnc_socket_t *out)
{
    (void)host;
    (void)port;
    (void)timeout_ms;
    *out = OVNC_INVALID_SOCKET;
    /* TODO: Win32 socket implementation */
    return OVNC_ERR_IO;
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
    return OVNC_ERR_IO;
}

ovnc_error_t ovnc__socket_recv_all(ovnc_socket_t sock,
                                   void *buf, size_t len)
{
    (void)sock;
    (void)buf;
    (void)len;
    return OVNC_ERR_IO;
}

#endif /* _WIN32 */
