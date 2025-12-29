/*
 * network.c
 * Network connection management
 * Author: Michael Czigler
 * License: MIT
 */

#include "network.h"

int network_send(network_t *network, const char *fmt, ...)
{
    if (network == NULL) {
        return -1;
    }

    if (network->transport == NULL) {
        return -1;
    }

    char buf[MESSAGE_MAX_LEN];
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

static void network_send_private_msg(network_t *network,
        char *msg)
{
    char msg_copy[MESSAGE_MAX_LEN];
    safecpy(msg_copy, msg, sizeof(msg_copy));

    char *username = strtok(msg_copy, " ");

    if (username == NULL) {
        const char *err = "error: message malformed";
        printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    char *message = strtok(NULL, "");

    if (message == NULL) {
        const char *err = "error: message malformed";
        printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    size_t overhead = strlen(username) + 13;
    size_t msg_len = strlen(message);
    
    if (overhead + msg_len >= MESSAGE_MAX_LEN) {
        const char *err = "error: message too long";
        printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    network_send(network, "PRIVMSG %s :%s\r\n",
        username, message);
    printf("\rto " BOLD_RED "%s" RESET ": %s" CLEAR_LINE "\r\n",
        username, message);
}

static void network_send_ctcp_action(network_t *network,
        char *msg)
{
    if (network->ctx->target[0] != '\0') {
        /* Validate total message length: "PRIVMSG " + target + " :\001ACTION " + msg + "\001\r\n" */
        size_t overhead = 10 + strlen(network->ctx->target) + 10 + 3; /* "PRIVMSG " + target + " :\001ACTION " + "\001\r\n" */
        size_t msg_len = strlen(msg);
        
        if (overhead + msg_len >= MESSAGE_MAX_LEN) {
            const char *err = "error: message too long";
            printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
            return;
        }

        network_send(network,
            "PRIVMSG %s :\001ACTION %s\001\r\n",
            network->ctx->target, msg);
        printf("\rto \u2022 " BOLD "%s" RESET ": %s" CLEAR_LINE "\r\n",
            network->ctx->target, msg);
    } else {
        const char *err = "error: no channel set";
        printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
    }
}

static void network_send_ctcp_command(network_t *network,
        char *msg)
{
    char msg_copy[MESSAGE_MAX_LEN];
    safecpy(msg_copy, msg, sizeof(msg_copy));

    char *target = strtok(msg_copy, " ");

    if (target == NULL) {
        const char *err = "usage: /ctcp <nick> <command>";
        printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }
 
    char *command = strtok(NULL, "");

    if (command == NULL) {
        const char *err = "usage: /ctcp <nick> <command>";
        printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    network_send(network, "PRIVMSG %s :\001%s\001\r\n",
        target, command);
    printf("\rctcp: " BOLD_RED "%s" RESET ": %s" CLEAR_LINE "\r\n",
        target, command);
}

static void network_send_channel_msg(
        network_t *network, char *msg)
{
    if (network->ctx->target[0] != '\0') {
        /* Validate total message length: "PRIVMSG " + target + " :" + msg + "\r\n" */
        size_t overhead = 10 + strlen(network->ctx->target) + 3 + 2; /* "PRIVMSG " + target + " :" + "\r\n" */
        size_t msg_len = strlen(msg);
        
        if (overhead + msg_len >= MESSAGE_MAX_LEN) {
            const char *err = "error: message too long";
            printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
            return;
        }

        network_send(network, "PRIVMSG %s :%s\r\n",
            network->ctx->target, msg);
        printf("\rto " BOLD "%s" RESET ": %s" CLEAR_LINE "\r\n",
            network->ctx->target, msg);
    } else {
        const char *err = "error: no channel set";
        printf("\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
    }
}

int network_command_handler(network_t *network, char *msg)
{
    switch (msg[0]) {
    case '/':  /* system command message */
        switch (msg[1]) {
        case 's':  /* set target (channel or nickname) */
            if (strncmp(msg + 1, "set ", 4) == 0) {
                size_t siz = sizeof(network->ctx->target);
                safecpy(network->ctx->target, msg + 5, siz);
            } else {
                network_send(network, "%s\r\n", msg + 1);
            }
            break;

        case 'm':  /* send CTCP ACTION to target */
            if (strncmp(msg + 1, "me ", 3) == 0) {
                network_send_ctcp_action(network, msg + 4);
            } else {
                network_send(network, "%s\r\n", msg + 1);
            }
            break;

        case 'c':  /* send CTCP command */
            if (strncmp(msg + 1, "ctcp ", 5) == 0) {
                network_send_ctcp_command(network, msg + 6);
            } else {
                network_send(network, "%s\r\n", msg + 1);
            }
            break;

        default:  /* send raw server command */
            network_send(network, "%s\r\n", msg + 1);
            break;  
        }
        break;

    case '@':  /* private message */
        network_send_private_msg(network, msg + 1);
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

    for (int offset = 0; offset < len; offset += AUTH_CHUNK_SIZE) {
        char chunk[AUTH_CHUNK_SIZE + 1];
        safecpy(chunk, network->ctx->auth + offset, sizeof(chunk));
        network_send(network, "AUTHENTICATE %s\r\n", chunk);
    }
    
    if ((len > 0) && (len % AUTH_CHUNK_SIZE == 0)) {
        network_send(network, "AUTHENTICATE +\r\n");
    }

    return 0;
}

int network_send_credentials(network_t *network)
{
    if (network->ctx->mechanism != SASL_NONE) {
        network_send(network, "CAP REQ :sasl\r\n");
    }

    network_send(network, "NICK %s\r\n",
        network->ctx->nickname);
    
    char *username, *realname;
    
    if (network->ctx->username[0] != '\0') {
        username = network->ctx->username;
    } else {
        username = network->ctx->nickname;
    }

    if (network->ctx->realname[0] != '\0') {
        realname = network->ctx->realname;
    } else {
        realname = network->ctx->nickname;
    }

    network_send(network, "USER %s - - :%s\r\n",
        username, realname);

    if (network->ctx->mechanism != SASL_NONE) {
        if (network->ctx->mechanism == SASL_EXTERNAL) {
            network_send(network, "AUTHENTICATE EXTERNAL\r\n");
        } else if (network->ctx->mechanism == SASL_PLAIN) {
            network_send(network, "AUTHENTICATE PLAIN\r\n");
        } else {
            fprintf(stderr, "unrecognized SASL mechanism\n");
            return -1;
        }
    }

    if (network->ctx->password[0] != '\0') {
        network_send(network, "PASS %s\r\n",
            network->ctx->password);
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
        transport_t *transport, kirc_context_t *ctx)
{
    memset(network, 0, sizeof(*network));

    network->ctx = ctx;
    network->transport = transport;

    return 0;
}

int network_free(network_t *network)
{
    if (transport_free(network->transport) < 0) {
        return -1;
    }

    return 0;
}
