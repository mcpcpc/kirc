#include "network.h"

static int poll_wait_write(int fd, int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;

    for (;;) {
        int rc = poll(&pfd, 1, timeout_ms);
        if (rc > 0)
            return 0;       /* ready */
        if (rc == 0)
            return -1;      /* timeout */
        if (errno == EINTR)
            continue;
        return -1;          /* error */
    }
}

static int poll_wait_read(int fd, int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;

    for (;;) {
        int rc = poll(&pfd, 1, timeout_ms);
        if (rc > 0)
            return 0;
        if (rc == 0)
            return -1;
        if (errno == EINTR)
            continue;
        return -1;
    }
}

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
    struct addrinfo hints, *res = NULL, *p = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;

    int status = getaddrinfo(network->ctx->server,
        network->ctx->port, &hints, &res);

    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n",
            gai_strerror(status));
        return -1;
    }

    network->fd = -1;

    for (p = res; p; p = p->ai_next) {
        int fd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol);

        if (fd < 0)
            continue;

        int flags = fcntl(fd, F_GETFL, 0);
        
        if (flags < 0) {
            close(fd);
            continue;
        }

        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        int rc = connect(fd, p->ai_addr, p->ai_addrlen);
        if (rc == 0) {
            network->fd = fd;
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
                    network->fd = fd;
                    break;
                }
            }
        }

        close(fd);
    }

    freeaddrinfo(res);

    if (network->fd < 0) {
        fprintf(stderr, "failed to connect\n");
        return -1;
    }

    return 0;
}

/*
int network_connect(network_t *network)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct addrinfo *p = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(network->ctx->server,
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

    // Set non-blocking
    int flags = fcntl(network->fd, F_GETFL, 0);

    if (flags != -1) {
        fcntl(network->fd, F_SETFL, flags | O_NONBLOCK);
    }

    return 0;
}
*/
int network_command_handler(network_t *network, char *msg)
{
    switch (msg[0]) {
    case '/':  /* system command message */
        switch (msg[1]) {
        case '#':  /* set active channel */
            int len = sizeof(network->ctx->selected) - 1;
            strncpy(network->ctx->selected, msg + 1, len);
            break;

        default:  /* send raw server command */
            network_send(network, "%s\r\n", msg + 1);
            break;  
        }
        break;

    case '@':  /* private message */
        char *username = strtok(msg + 1, " ");
        char *message = strtok(NULL, "");
        if (username && message) {
            network_send(network, "PRIVMSG %s :%s\r\n",
                username, message);
            printf("\rto \x1b[1;31m%s\x1b[0m: %s\x1b[0K\r\n",
                username, message);
        }
        break;

    default:  /* channel message */
        if (network->ctx->selected[0] != '\0') {
            network_send(network, "PRIVMSG %s :%s\r\n",
                network->ctx->selected, msg);
            printf("\rto \x1b[1m%s\x1b[0m: %s\x1b[0K\r\n",
                network->ctx->selected, msg);
        } else {
            char *err = "error: no channel set";
            printf("\r\x1b[0K\x1b[2m%s\x1b[0m\r\n", err);
        }
        break;
    }

    return 0;
}

static int network_authenticate_plain(network_t *network)
{
    if (network->ctx->auth[0] == '\0') {
        network_send(network, "AUTHENTICATE '*'\r\n");
        return -1;
    }
    
    int len = strlen(network->ctx->auth);
    int chunk_size = IRCV3_AUTHENTICATE_CHUNK_SIZE;

    for (int offset = 0; offset < len; offset += chunk_size) {
        char chunk[chunk_size + 1];
        strncpy(chunk, network->ctx->auth + offset, chunk_size);
        chunk[chunk_size] = '\0';
        network_send(network, "AUTHENTICATE %s\r\n", chunk);
    }
    
    if (len % chunk_size == 0) {
        network_send(network, "AUTHENTICATE +\r\n");
    }

    return 0;
}

int network_authenticate(network_t *network)
{
    switch (network->ctx->mechanism) {
    case SASL_PLAIN:
        network_authenticate_plain(network);
        break;
    
    case SASL_EXTERNAL:
        network_send(network, "AUTHENTICATE +\r\n");
        break;
    
    default:
        network_send(network, "AUTHENTICATE '*'\r\n");
        return -1;
        break;
    }
    
    network_send(network, "CAP END\r\n");

    return 0;
}

int network_join_channels(network_t *network)
{
    for (int i = 0; network->ctx->channels[i][0] != '\0'; ++i) {
        network_send(network, "JOIN %s\r\n",
            network->ctx->channels[i]);
    }

    return 0;
}

int network_init(network_t *network, kirc_t *ctx)
{
    memset(network, 0, sizeof(*network));   

    network->ctx = ctx;

    network->fd = -1;
    network->len = 0;

    return 0;
}

int network_free(network_t *network)
{
    if (network->fd != -1) {
        close(network->fd);
        network->fd = -1;
    }

    return 0;
}
