#include "transport.h"

static int poll_wait_write(int fd, int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;

    for (;;) {
        int rc = poll(&pfd, 1, timeout_ms);
        if (rc > 0)
            return 0;  /* ready */
        if (rc == 0)
            return -1;  /* timeout */
        if (errno == EINTR)
            continue;
        return -1;  /* error */
    }
}

ssize_t transport_send(transport_t *transport,
        const char *buffer, size_t len)
{
    if (transport == NULL)
        return -1;

    if (transport->fd < 0)
        return -1;

    ssize_t rc = write(transport->fd,
        buffer, len);

    if (rc < 0)
        return -1;

    return rc;
}

ssize_t transport_receive(transport_t *transport,
        char *buffer, size_t len)
{
    if (transport == NULL)
        return -1;

    if (transport->fd < 0)
        return -1;

    ssize_t nread = read(transport->fd,
        buffer, len);

    if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            return -1;
        } 
    }

    if (nread == 0) {
        return -1; 
    }

    return nread;
}


int transport_connect(transport_t *transport)
{
    struct addrinfo hints, *res = NULL, *p = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    int status = getaddrinfo(transport->ctx->server,
        transport->ctx->port, &hints, &res);

    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",
            gai_strerror(status));
        return -1;
    }

    transport->fd = -1;

    for (p = res; p; p = p->ai_next) {
        int fd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol);

        if (fd < 0) {
            continue;
        }

        int flags = fcntl(fd, F_GETFL, 0);
        
        if (flags < 0) {
            close(fd);
            continue;
        }

        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        int rc = connect(fd, p->ai_addr, p->ai_addrlen);

        if (rc == 0) {
            transport->fd = fd;
            break;
        }

        if (errno == EINPROGRESS) {
            int timeout_ms = KIRC_TIMEOUT_MS;
            int rc = poll_wait_write(fd, timeout_ms);

            if (rc == 0) {

                int soerr = 0;
                socklen_t slen = sizeof(soerr);

                getsockopt(fd, SOL_SOCKET, SO_ERROR,
                    &soerr, &slen);

                if (soerr == 0) {
                    transport->fd = fd;
                    break;
                }
            }
        }

        close(fd);
    }

    freeaddrinfo(res);

    if (transport->fd < 0) {
        return -1;
    }

    return 0;
}

int transport_init(transport_t *transport,
        kirc_t *ctx)
{
    memset(transport, 0, sizeof(*transport));

    transport->ctx = ctx;
    transport->fd = -1;

    return 0;
}

int transport_free(transport_t *transport)
{
    if (transport == NULL) {
        return -1;
    }

    if (transport->fd != -1) {
        close(transport->fd);
        transport->fd = -1;
    }
    return 0;
}
