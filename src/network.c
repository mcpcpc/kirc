#include "network.h"

int network_send(network_t *network, const char *fmt, ...)
{
    if (network == NULL) {
        return -1;
    }

    if (network->transport == NULL) {
        return -1;
    }

    char buf[RFC1459_MESSAGE_MAX_LEN];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    size_t len = strnlen(buf, sizeof(buf));

    ssize_t nsent = transport_send(
        network->transport, buf, len);

    if (nsent < 0) {
        return -1;
    }

    return 0;
}

int network_receive(network_t *network)
{
    size_t buffer_n = sizeof(network->buffer) - 1;
    ssize_t nread = transport_receive(
        network->transport,
        network->buffer + network->len,
        buffer_n - network->len);

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

    network->len += (int)nread;
    network->buffer[network->len] = '\0';

    return nread;
}

int network_connect(network_t *network)
{
    return transport_connect(network->transport);
}

static void network_send_private_msg(
        network_t *network, char *msg)
{
    char *username = strtok(msg + 1, " ");
    char *message = strtok(NULL, "");

    if (username && message) {
        network_send(network, "PRIVMSG %s :%s\r\n",
            username, message);
        printf("\rto " ANSI_BOLD_RED "%s" ANSI_RESET
            ": %s" ANSI_CLEAR_LINE "\r\n",
            username, message);
    } else {
        const char *err = "error: missing nickname or message";
        printf("\r" ANSI_CLEAR_LINE ANSI_DIM "%s" ANSI_RESET
            "\r\n", err);
    }
}

static void network_send_channel_msg(
        network_t *network, char *msg)
{
    if (network->ctx->selected[0] != '\0') {
        network_send(network, "PRIVMSG %s :%s\r\n",
            network->ctx->selected, msg);
        printf("\rto " ANSI_BOLD "%s" ANSI_RESET
            ": %s" ANSI_CLEAR_LINE "\r\n",
            network->ctx->selected, msg);
    } else {
        const char *err = "error: no channel set";
        printf("\r" ANSI_CLEAR_LINE ANSI_DIM "%s" ANSI_RESET
            "\r\n", err);
    }
}

int network_command_handler(network_t *network, char *msg)
{
    size_t siz = 0;

    switch (msg[0]) {
    case '/':  /* system command message */
        switch (msg[1]) {
        case '#':  /* set active channel */
            siz = sizeof(network->ctx->selected) - 1;
            strncpy(network->ctx->selected, msg + 1, siz);
            break;

        default:  /* send raw server command */
            network_send(network, "%s\r\n", msg + 1);
            break;  
        }
        break;

    case '@':  /* private message */
        network_send_private_msg(network, msg);
        break;

    default:  /* channel message */
        network_send_channel_msg(network, msg);
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
    int chunk_size = AUTHENTICATE_CHUNK_SIZE;

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

int network_init(network_t *network, 
        transport_t *transport, kirc_t *ctx)
{
    memset(network, 0, sizeof(*network));

    network->ctx = ctx;
    network->transport = transport;
    network->len = 0;

    return 0;
}

int network_free(network_t *network)
{
    if (transport_free(network->transport) < 0) {
        return -1;
    }

    return 0;
}
