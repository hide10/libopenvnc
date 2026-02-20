#include "ovnc_platform.h"

#ifndef _WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <poll.h>

ovnc_error_t ovnc__platform_init(void)
{
    return OVNC_OK;
}

void ovnc__platform_cleanup(void)
{
}

ovnc_error_t ovnc__socket_connect(const char *host, uint16_t port,
                                  uint32_t timeout_ms, ovnc_socket_t *out)
{
    struct addrinfo hints, *res, *rp;
    char port_str[6];
    int sock;
    ovnc_error_t err = OVNC_ERR_CONNECTION_REFUSED;

    *out = OVNC_INVALID_SOCKET;
    snprintf(port_str, sizeof(port_str), "%u", (unsigned)port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int gai_err = getaddrinfo(host, port_str, &hints, &res);
    if (gai_err != 0) {
        return OVNC_ERR_CONNECTION_REFUSED;
    }

    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock < 0)
            continue;

        if (timeout_ms > 0) {
            int flags = fcntl(sock, F_GETFL, 0);
            if (flags >= 0)
                fcntl(sock, F_SETFL, flags | O_NONBLOCK);

            int rc = connect(sock, rp->ai_addr, rp->ai_addrlen);
            if (rc < 0 && errno == EINPROGRESS) {
                struct pollfd pfd;
                pfd.fd = sock;
                pfd.events = POLLOUT;
                int poll_rc = poll(&pfd, 1, (int)timeout_ms);
                if (poll_rc <= 0) {
                    close(sock);
                    err = (poll_rc == 0) ? OVNC_ERR_TIMEOUT : OVNC_ERR_IO;
                    continue;
                }
                int so_error = 0;
                socklen_t so_len = sizeof(so_error);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &so_len);
                if (so_error != 0) {
                    close(sock);
                    err = OVNC_ERR_CONNECTION_REFUSED;
                    continue;
                }
            } else if (rc < 0) {
                close(sock);
                continue;
            }

            /* Restore blocking mode */
            if (flags >= 0)
                fcntl(sock, F_SETFL, flags);
        } else {
            if (connect(sock, rp->ai_addr, rp->ai_addrlen) < 0) {
                close(sock);
                continue;
            }
        }

        /* Success */
        freeaddrinfo(res);
        *out = sock;
        return OVNC_OK;
    }

    freeaddrinfo(res);
    return err;
}

void ovnc__socket_close(ovnc_socket_t sock)
{
    if (sock != OVNC_INVALID_SOCKET)
        close(sock);
}

ovnc_error_t ovnc__socket_send_all(ovnc_socket_t sock,
                                   const void *data, size_t len)
{
    const uint8_t *p = (const uint8_t *)data;
    size_t remaining = len;

    while (remaining > 0) {
        ssize_t n = send(sock, p, remaining, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return OVNC_ERR_IO;
        }
        if (n == 0)
            return OVNC_ERR_CONNECTION_CLOSED;
        p += n;
        remaining -= (size_t)n;
    }
    return OVNC_OK;
}

ovnc_error_t ovnc__socket_recv_all(ovnc_socket_t sock,
                                   void *buf, size_t len)
{
    uint8_t *p = (uint8_t *)buf;
    size_t remaining = len;

    while (remaining > 0) {
        ssize_t n = recv(sock, p, remaining, 0);
        if (n < 0) {
            if (errno == EINTR)
                continue;
            return OVNC_ERR_IO;
        }
        if (n == 0)
            return OVNC_ERR_CONNECTION_CLOSED;
        p += n;
        remaining -= (size_t)n;
    }
    return OVNC_OK;
}

#endif /* !_WIN32 */
