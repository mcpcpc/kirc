#include "network.h"

void network_send(network_t *network, const char *fmt, ...)
{
    char buf[RFC1459_MESSAGE_MAX_LEN];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    size_t len = strnlen(buf, sizeof(buf));

    if (write(network->fd, buf, len) < 0) {
        perror("write");
    }
}

int network_receive(network_t *network) {
    size_t buffer_n = sizeof(network->buffer) - 1;
    ssize_t nread = read(network->fd,
        network->buffer + network->len,
        buffer_n - network->len);

    if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        } else {
            perror("read");
            return -1;
        }
    }

    if (nread == 0) {
        return -1;
    }

    network->len += (int)nread;
    network->buffer[network->len] = '\0';

    return nread;
}

int network_connect(network_t *network)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *p = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(network->ctx->hostname,
        network->ctx->port, &hints, &res);

    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",
            gai_strerror(status));
        return -1;
    }

    network->fd = -1;

    for (p = res; p != NULL; p = p->ai_next) {
        network->fd = socket(p->ai_family,
            p->ai_socktype, p->ai_protocol);

        if (network->fd == -1) {
            continue;
        }

        if (connect(network->fd, p->ai_addr,
            p->ai_addrlen) == -1) {
            network->fd = -1;
            close(network->fd);
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (network->fd == -1) {
        fprintf(stderr, "failed to connect\n");
        return -1;
    }

    /* Set non-blocking */
    int flags = fcntl(network->fd, F_GETFL, 0);

    if (flags != -1) {
        fcntl(network->fd, F_SETFL, flags | O_NONBLOCK);
    }

    return 0;
}

int network_command_handler(network_t *network, char *msg)
{
    switch (msg[0]) {
    case '/':  /* system command message */
        if (msg[1] == '#') {
            int len = sizeof(network->ctx->active) - 1;
            strncpy(network->ctx->active, msg + 1, len);
        } else {
            network_send(network, "%s\r\n", msg + 1);
        }
        break;

    case '@':  /* private message */
        break;

    default:  /* channel message */
        break;
    }
}

int network_init(network_t *network, kirc_t *ctx)
{
    memset(network, 0, sizeof(*network));   

    network->ctx = ctx;

    network->fd = -1;
    network->len = 0;

    return 0;
}
