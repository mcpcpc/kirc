#include "network.h"

int network_connect(network_t *net)
{
    if (!net || !net->host || !net->port) {
        errno = EINVAL;
        return -1;
    }

    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *p   = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int gai_status = getaddrinfo(net->host, net->port, &hints, &res);
    if (gai_status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",
                gai_strerror(gai_status));
        return -1;
    }

    int sock = -1;

    for (p = res; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family,
                      p->ai_socktype,
                      p->ai_protocol);
        if (sock == -1) {
            continue;
        }

        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock);
            sock = -1;
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (sock == -1) {
        fprintf(stderr, "network_connect: failed to connect\n");
        return -1;
    }

    /* Set non-blocking */
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags != -1) {
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
    }

    net->socket_fd = sock;
    net->rx_len    = 0;

    return 0;
}

void network_send(network_t *net, const char *fmt, ...)
{
    if (!net || net->socket_fd < 0 || !fmt) {
        return;
    }

    char buf[MSG_MAX];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    size_t len = strnlen(buf, sizeof(buf));

    ssize_t nw = write(net->socket_fd, buf, len);
    if (nw < 0) {
        perror("network_send: write");
    }
}

int network_poll(network_t *net)
{
    if (!net || net->socket_fd < 0) {
        return -2;
    }

    if (net->rx_len >= sizeof(net->rx_buffer) - 1) {
        /* Buffer full, drop contents safely */
        net->rx_len = 0;
    }

    ssize_t nread = read(net->socket_fd,
                          net->rx_buffer + net->rx_len,
                          (sizeof(net->rx_buffer) - 1) - net->rx_len);

    if (nread == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;  /* no data */
        }
        perror("network_poll: read");
        return -2;
    }

    if (nread == 0) {
        /* orderly shutdown by peer */
        return -1;
    }

    net->rx_len += (size_t)nread;
    net->rx_buffer[net->rx_len] = '\0';

    return 1;
}
