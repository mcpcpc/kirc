#include "network.h"

void network_send(kirc_t *ctx, const char *fmt, ...)
{
    char buf[RFC1459_MESSAGE_MAX_LEN];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    size_t len = strnlen(buf, sizeof(buf));

    if (write(ctx->conn, buf, len) < 0) {
        perror("write");
    }
}

int network_connect(kirc_t *ctx)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *p = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(ctx->hostname, ctx->port,
        &hints, &res);

    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    ctx->conn = -1;

    for (p = res; p != NULL; p = p->ai_next) {
        ctx->conn = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol);

        if (ctx->conn == -1) {
            continue;
        }

        if (connect(ctx->conn, p->ai_addr, p->ai_addrlen) == -1) {
            ctx->conn = -1;
            close(ctx->conn);
            continue;
        }

        break;
    }

    freeaddrinfo(res);

    if (ctx->conn == -1) {
        fprintf(stderr, "failed to connect\n");
        return -1;
    }

    /* Set non-blocking */
    int flags = fcntl(ctx->conn, F_GETFL, 0);
    if (flags != -1) {
        fcntl(ctx->conn, F_SETFL, flags | O_NONBLOCK);
    }

    return 0;
}

void network_poll(kirc_t *ctx) {
    
}
