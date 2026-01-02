/*
 * network.c
 * Network connection management
 * Author: Michael Czigler
 * License: MIT
 */

#include "network.h"

/**
 * network_send() - Send formatted message to IRC server
 * @network: Network connection structure
 * @fmt: printf-style format string
 * @...: Variable arguments for format string
 *
 * Constructs and sends a formatted message to the IRC server through the
 * transport layer. Messages are limited to MESSAGE_MAX_LEN bytes.
 *
 * Return: 0 on success, -1 if network is NULL or send fails
 */
int network_send(struct network *network, const char *fmt, ...)
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

/**
 * network_receive() - Receive data from IRC server
 * @network: Network connection structure
 *
 * Reads available data from the server into the network buffer. Handles
 * non-blocking I/O and partial reads. Updates the buffer length with
 * newly received data. Returns immediately if no data is available.
 *
 * Return: Number of bytes received, 0 if would block, -1 on error or disconnect
 */
int network_receive(struct network *network)
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

/**
 * network_connect() - Establish connection to IRC server
 * @network: Network connection structure
 *
 * Initiates connection to the IRC server using the transport layer.
 * Connection details (server, port) are taken from the context.
 *
 * Return: 0 on success, -1 on connection failure
 */
int network_connect(struct network *network)
{
    return transport_connect(network->transport);
}

/**
 * network_send_private_msg() - Send private message to user
 * @network: Network connection structure
 * @msg: Message string in format "username message text"
 * @output: Output buffer for display feedback
 *
 * Parses and sends a private message (PRIVMSG) to a specific user.
 * Expected format: "username message text". Validates message length
 * and displays sent message to user.
 */
static void network_send_private_msg(struct network *network,
        char *msg, struct output *output)
{
    char msg_copy[MESSAGE_MAX_LEN];
    safecpy(msg_copy, msg, sizeof(msg_copy));

    char *username = strtok(msg_copy, " ");

    if (username == NULL) {
        const char *err = "error: message malformed";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    char *message = strtok(NULL, "");

    if (message == NULL) {
        const char *err = "error: message malformed";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    size_t overhead = strlen(username) + 13;
    size_t msg_len = strlen(message);
    
    if (overhead + msg_len >= MESSAGE_MAX_LEN) {
        const char *err = "error: message too long";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    network_send(network, "PRIVMSG %s :%s\r\n",
        username, message);
    output_append(output, "\rto " BOLD_RED "%s" RESET ": %s" CLEAR_LINE "\r\n",
        username, message);
}

/**
 * network_send_topic() - Send TOPIC command for channel
 * @network: Network connection structure
 * @msg: Topic command arguments (channel and optional new topic)
 * @output: Output buffer for error messages
 *
 * Parses and sends IRC TOPIC command. If only channel is provided,
 * requests the current topic. If topic text is included, sets a new topic.
 * Displays usage error if channel is missing.
 */
static void network_send_topic(struct network *network,
        char *msg, struct output *output)
{
    char msg_copy[MESSAGE_MAX_LEN];
    safecpy(msg_copy, msg, sizeof(msg_copy));

    char *channel = strtok(msg_copy, " ");

    if (channel == NULL) {
        const char *err = "usage: /topic <channel> [<topic>]";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }
 
    char *topic = strtok(NULL, "");

    if (topic == NULL) {
        network_send(network, "TOPIC %s\r\n", channel);
        return;
    }

    network_send(network, "TOPIC %s :%s\r\n", channel, topic);
}

/**
 * network_send_ctcp_action() - Send CTCP ACTION to current target
 * @network: Network connection structure
 * @msg: Action text to send
 * @output: Output buffer for display feedback
 *
 * Sends a CTCP ACTION message (/me command) to the current target channel
 * or user. Validates message length and requires a target to be set.
 */
static void network_send_ctcp_action(struct network *network,
        char *msg, struct output *output)
{
    if (network->ctx->target[0] != '\0') {
        /* Validate total message length: "PRIVMSG " + target + " :\001ACTION " + msg + "\001\r\n" */
        size_t overhead = 10 + strlen(network->ctx->target) + 10 + 3; /* "PRIVMSG " + target + " :\001ACTION " + "\001\r\n" */
        size_t msg_len = strlen(msg);
        
        if (overhead + msg_len >= MESSAGE_MAX_LEN) {
            const char *err = "error: message too long";
            output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
            return;
        }

        network_send(network,
            "PRIVMSG %s :\001ACTION %s\001\r\n",
            network->ctx->target, msg);
        output_append(output, "\rto \u2022 " BOLD "%s" RESET ": %s" CLEAR_LINE "\r\n",
            network->ctx->target, msg);
    } else {
        const char *err = "error: no channel set";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
    }
}

/**
 * network_send_ctcp_command() - Send CTCP command to target
 * @network: Network connection structure
 * @msg: Message in format "target command [args]"
 * @output: Output buffer for display feedback
 *
 * Parses and sends a CTCP command to a specified target. Expected format:
 * "target command [args]". Validates input and provides usage feedback.
 */
static void network_send_ctcp_command(struct network *network,
        char *msg, struct output *output)
{
    char msg_copy[MESSAGE_MAX_LEN];
    safecpy(msg_copy, msg, sizeof(msg_copy));

    char *target = strtok(msg_copy, " ");

    if (target == NULL) {
        const char *err = "usage: /ctcp <nick> <command>";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }
 
    char *command = strtok(NULL, "");

    if (command == NULL) {
        const char *err = "usage: /ctcp <nick> <command>";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
        return;
    }

    network_send(network, "PRIVMSG %s :\001%s\001\r\n",
        target, command);
    output_append(output, "\rctcp: " BOLD_RED "%s" RESET ": %s" CLEAR_LINE "\r\n",
        target, command);
}

/**
 * network_send_channel_msg() - Send message to current target channel
 * @network: Network connection structure
 * @msg: Message text to send
 * @output: Output buffer for display feedback
 *
 * Sends a channel message to the currently set target. Validates message
 * length and requires a target to be set. Displays sent message locally.
 */
static void network_send_channel_msg(
        struct network *network, char *msg, struct output *output)
{
    if (network->ctx->target[0] != '\0') {
        /* Validate total message length: "PRIVMSG " + target + " :" + msg + "\r\n" */
        size_t overhead = 10 + strlen(network->ctx->target) + 3 + 2; /* "PRIVMSG " + target + " :" + "\r\n" */
        size_t msg_len = strlen(msg);
        
        if (overhead + msg_len >= MESSAGE_MAX_LEN) {
            const char *err = "error: message too long";
            output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
            return;
        }

        network_send(network, "PRIVMSG %s :%s\r\n",
            network->ctx->target, msg);
        output_append(output, "\rto " BOLD "%s" RESET ": %s" CLEAR_LINE "\r\n",
            network->ctx->target, msg);
    } else {
        const char *err = "error: no channel set";
        output_append(output, "\r" CLEAR_LINE DIM "%s" RESET "\r\n", err);
    }
}

/**
 * network_command_handler() - Process user input commands
 * @network: Network connection structure
 * @msg: User input message to process
 * @output: Output buffer for display feedback
 *
 * Routes user input to appropriate handlers based on prefix:
 *   / - IRC commands (/set, /me, /ctcp, or raw IRC commands)
 *   @ - Private messages to users
 *   (default) - Channel messages to current target
 * Parses commands and delegates to specialized send functions.
 *
 * Return: 0 on success
 */
int network_command_handler(struct network *network, char *msg, struct output *output)
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

        case 't':  /* send TOPIC command */
            if (strncmp(msg + 1, "topic ", 6) == 0) {
                network_send_topic(network, msg + 7, output);
            } else {
                network_send(network, "%s\r\n", msg + 1);
            }
            break;

        case 'm':  /* send CTCP ACTION to target */
            if (strncmp(msg + 1, "me ", 3) == 0) {
                network_send_ctcp_action(network, msg + 4, output);
            } else {
                network_send(network, "%s\r\n", msg + 1);
            }
            break;

        case 'c':  /* send CTCP command */
            if (strncmp(msg + 1, "ctcp ", 5) == 0) {
                network_send_ctcp_command(network, msg + 6, output);
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
        network_send_private_msg(network, msg + 1, output);
        break;

    default:  /* channel message */
        network_send_channel_msg(network, msg, output);
        break;
    }

    return 0;
}

/**
 * network_send_credentials() - Send authentication credentials to server
 * @network: Network connection structure
 *
 * Sends initial authentication sequence to IRC server including SASL
 * capability negotiation (if configured), NICK, USER commands, and
 * optionally PASS (server password). Initiates SASL authentication
 * for EXTERNAL or PLAIN mechanisms.
 *
 * Return: 0 on success, -1 if SASL mechanism is unrecognized
 */
int network_send_credentials(struct network *network)
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

/**
 * network_init() - Initialize network connection structure
 * @network: Network structure to initialize
 * @transport: Transport layer instance
 * @ctx: IRC context structure
 *
 * Initializes the network management structure, associating it with a
 * transport layer and IRC context. Zeros the buffer and state.
 *
 * Return: 0 on success, -1 if any parameter is NULL
 */
int network_init(struct network *network, 
        struct transport *transport, struct kirc_context *ctx)
{
    if ((network == NULL) || (transport == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(network, 0, sizeof(*network));

    network->ctx = ctx;
    network->transport = transport;

    return 0;
}

/**
 * network_free() - Free network resources
 * @network: Network structure to clean up
 *
 * Releases resources associated with the network connection by freeing
 * the transport layer.
 *
 * Return: 0 on success, -1 if transport cleanup fails
 */
int network_free(struct network *network)
{
    if (transport_free(network->transport) < 0) {
        return -1;
    }

    return 0;
}
