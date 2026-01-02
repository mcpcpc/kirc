/*
 * transport.c
 * Data transport layer
 * Author: Michael Czigler
 * License: MIT
 */

#include "transport.h"

/**
 * poll_wait_write() - Wait for socket to be writable
 * @fd: File descriptor to poll
 * @timeout_ms: Timeout in milliseconds
 *
 * Polls a file descriptor for write readiness, handling EINTR by retrying.
 * Used during non-blocking connect() to wait for connection completion.
 *
 * Return: 0 if ready for write, -1 on timeout or error
 */
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

/**
 * transport_send() - Send data over transport connection
 * @transport: Transport structure
 * @buffer: Data buffer to send
 * @len: Number of bytes to send
 *
 * Writes data to the transport file descriptor. Does not guarantee
 * that all data is sent in one call (partial writes possible).
 *
 * Return: Number of bytes written, or -1 on error
 */
ssize_t transport_send(struct transport *transport,
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

/**
 * transport_receive() - Receive data from transport connection
 * @transport: Transport structure
 * @buffer: Buffer to store received data
 * @len: Maximum number of bytes to receive
 *
 * Reads data from the transport file descriptor. Handles non-blocking
 * I/O by distinguishing between EAGAIN/EWOULDBLOCK (no data available)
 * and other errors. Returns -1 on connection close (nread == 0).
 *
 * Return: Number of bytes read, 0 if would block, -1 on error or disconnect
 */
ssize_t transport_receive(struct transport *transport,
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


/**
 * transport_connect() - Establish transport connection to server
 * @transport: Transport structure with server details
 *
 * Creates a socket and connects to the server specified in the transport
 * context (server name and port). Uses getaddrinfo() for address resolution,
 * supporting both IPv4 and IPv6. Configures socket for non-blocking I/O
 * and handles EINPROGRESS for asynchronous connection with timeout.
 *
 * Return: 0 on successful connection, -1 on failure
 */
int transport_connect(struct transport *transport)
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

        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(fd);
            continue;
        }

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

/**
 * transport_init() - Initialize transport structure
 * @transport: Transport structure to initialize
 * @ctx: IRC context containing connection details
 *
 * Initializes the transport layer, zeroing the structure and setting
 * the file descriptor to -1 (not connected). Associates transport with
 * IRC context for server and port information.
 *
 * Return: 0 on success, -1 if transport or ctx is NULL
 */
int transport_init(struct transport *transport,
        struct kirc_context *ctx)
{
    if ((transport == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(transport, 0, sizeof(*transport));

    transport->ctx = ctx;
    transport->fd = -1;

    return 0;
}

/**
 * transport_free() - Close and cleanup transport connection
 * @transport: Transport structure to free
 *
 * Closes the transport socket if open and resets the file descriptor
 * to -1. Should be called during cleanup to release resources.
 *
 * Return: 0 on success, -1 if transport is NULL
 */
int transport_free(struct transport *transport)
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
