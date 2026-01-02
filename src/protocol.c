/*
 * protocol.c
 * IRC protocol handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "protocol.h"

/**
 * protocol_get_time() - Get formatted timestamp string
 *
 * Returns the current local time as a formatted string using the format
 * defined by KIRC_TIMESTAMP_FORMAT. Uses a static buffer, so the returned
 * pointer is valid until the next call.
 *
 * Return: Pointer to static timestamp string
 */
static const char *protocol_get_time(void)
{
    static char timestamp[KIRC_TIMESTAMP_SIZE];
    time_t current;
    time(&current);
    struct tm *info = localtime(&current);
    strftime(timestamp, KIRC_TIMESTAMP_SIZE,
        KIRC_TIMESTAMP_FORMAT, info);
    return timestamp;
}

/**
 * protocol_noop() - No-operation event handler
 * @network: Network connection (unused)
 * @event: Event structure (unused)
 * @output: Output buffer (unused)
 *
 * Handler that does nothing. Used for events that should be silently
 * ignored (e.g., JOIN, PART, QUIT).
 */
void protocol_noop(struct network *network, struct event *event, struct output *output)
{
    (void)network;
    (void)event;
    (void)output;
}

/**
 * protocol_ping() - Handle PING requests from server
 * @network: Network connection structure
 * @event: Event containing ping data
 * @output: Output buffer (unused)
 *
 * Responds to server PING messages with appropriate PONG reply to
 * maintain connection keepalive.
 */
void protocol_ping(struct network *network, struct event *event, struct output *output)
{
    (void)output;
    network_send(network, "PONG :%s\r\n", event->message);
}

/**
 * network_authenticate_plain() - Send SASL PLAIN authentication
 * @network: Network connection structure
 *
 * Sends base64-encoded SASL PLAIN authentication data in chunks of
 * AUTH_CHUNK_SIZE bytes. If the total length is an exact multiple of
 * the chunk size, sends a final "+" to indicate completion.
 */
static void network_authenticate_plain(struct network *network)
{   
    int len = strlen(network->ctx->auth);

    for (int offset = 0; offset < len; offset += AUTH_CHUNK_SIZE) {
        char chunk[AUTH_CHUNK_SIZE + 1];
        safecpy(chunk, network->ctx->auth + offset, sizeof(chunk));
        network_send(network, "AUTHENTICATE %s\r\n", chunk);
    }
    
    if ((len > 0) && (len % AUTH_CHUNK_SIZE == 0)) {
        network_send(network, "AUTHENTICATE +\r\n");
    }
}

/**
 * protocol_authenticate() - Handle SASL authentication challenge
 * @network: Network connection structure
 * @event: Authentication event (unused)
 * @output: Output buffer (unused)
 *
 * Responds to server's AUTHENTICATE challenge based on configured SASL
 * mechanism (PLAIN or EXTERNAL). Sends authentication data and finalizes
 * capability negotiation with CAP END.
 */
void protocol_authenticate(struct network *network, struct event *event, struct output *output)
{
    (void)event;
    (void)output;

    if (network->ctx->auth[0] == '\0') {
        network_send(network, "AUTHENTICATE '*'\r\n");
        return;
    }

    switch (network->ctx->mechanism) {
    case SASL_PLAIN:
        network_authenticate_plain(network);
        break;
    
    case SASL_EXTERNAL:
        network_send(network, "AUTHENTICATE +\r\n");
        break;
    
    default:
        network_send(network, "AUTHENTICATE '*'\r\n");
        break;
    }
    
    if (network->ctx->mechanism != SASL_NONE) {
        network_send(network, "CAP END\r\n");
    }
}

/**
 * protocol_welcome() - Handle RPL_WELCOME (001) server message
 * @network: Network connection structure
 * @event: Welcome event (unused)
 * @output: Output buffer (unused)
 *
 * Processes the server welcome message by automatically joining all
 * configured channels from the context.
 */
void protocol_welcome(struct network *network, struct event *event, struct output *output)
{
    (void)event;
    (void)output;

    for (int i = 0; network->ctx->channels[i][0] != '\0'; ++i) {
        network_send(network, "JOIN %s\r\n",
            network->ctx->channels[i]);
    }
}

/**
 * protocol_raw() - Display raw IRC message
 * @network: Network connection (unused)
 * @event: Event containing raw message
 * @output: Output buffer for display
 *
 * Displays the complete raw IRC message with timestamp. Used as the
 * default handler for events without specific handlers.
 */
void protocol_raw(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    output_append(output, "\r" CLEAR_LINE DIM "%s" RESET
        " " REVERSE "%s" RESET "\r\n",
        protocol_get_time(), event->raw);
}

/**
 * protocol_info() - Display informational message
 * @network: Network connection (unused)
 * @event: Event containing message
 * @output: Output buffer for display
 *
 * Displays an informational IRC message with timestamp in dimmed text.
 * Used for most server replies and status messages.
 */
void protocol_info(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    output_append(output, "\r" CLEAR_LINE DIM "%s %s" RESET "\r\n",
        protocol_get_time(), event->message);
}

/**
 * protocol_error() - Display error message
 * @network: Network connection (unused)
 * @event: Event containing error message
 * @output: Output buffer for display
 *
 * Displays an IRC error message with timestamp. Error text is shown
 * in bold red to distinguish from normal messages.
 */
void protocol_error(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    output_append(output, "\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_RED "%s" RESET "\r\n",
        protocol_get_time(), event->message);
}

/**
 * protocol_notice() - Display NOTICE message
 * @network: Network connection (unused)
 * @event: Event containing notice details
 * @output: Output buffer for display
 *
 * Displays a NOTICE message with the sender's nickname in bold blue
 * and timestamp.
 */
void protocol_notice(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    output_append(output, "\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " %s\r\n",
        protocol_get_time(), event->nickname, event->message);
}

/**
 * protocol_privmsg_direct() - Display direct private message
 * @network: Network connection (unused)
 * @event: Event containing private message
 * @output: Output buffer for display
 *
 * Displays a private message sent directly to the user (not in a channel).
 * Shows sender in bold blue with message text in blue.
 */
static void protocol_privmsg_direct(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    output_append(output, "\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " " BLUE "%s" RESET "\r\n",
        protocol_get_time(), event->nickname, event->message);
}

/**
 * protocol_privmsg_indirect() - Display channel message
 * @network: Network connection (unused)
 * @event: Event containing channel message
 * @output: Output buffer for display
 *
 * Displays a message sent to a channel. Shows nickname in bold,
 * channel name in brackets, and message text.
 */
static void protocol_privmsg_indirect(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    output_append(output, "\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD "%s" RESET " [%s]: %s\r\n",
        protocol_get_time(), event->nickname, event->channel, event->message);

}

/**
 * protocol_privmsg() - Route PRIVMSG to appropriate handler
 * @network: Network connection structure
 * @event: Event containing PRIVMSG details
 * @output: Output buffer for display
 *
 * Determines whether a PRIVMSG is a direct message or channel message
 * by comparing the target with the user's nickname, then routes to the
 * appropriate display function.
 */
void protocol_privmsg(struct network *network, struct event *event, struct output *output)
{
    char *channel = event->channel;
    char *nickname = event->ctx->nickname;

    if (strcmp(channel, nickname) == 0) {
        protocol_privmsg_direct(network, event, output);
    } else {
        protocol_privmsg_indirect(network, event, output);
    }
}

/**
 * protocol_nick() - Handle nickname change events
 * @network: Network connection (unused)
 * @event: Event containing nickname change information
 * @output: Output buffer for display
 *
 * Handles NICK events by displaying the nickname change. If the change
 * is for the current user, updates the context with the new nickname.
 * Shows appropriate messages for self and other users.
 */
void protocol_nick(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    struct kirc_context *ctx = event->ctx;
    const char *timestamp = protocol_get_time();
    
    if (strcmp(event->nickname, ctx->nickname) == 0) {
        size_t siz = sizeof(ctx->nickname) - 1;
        strncpy(ctx->nickname, event->message, siz);
        ctx->nickname[siz] = '\0';
        output_append(output, "\r" CLEAR_LINE
            DIM "%s you are now known as %s" RESET "\r\n",
            timestamp, event->message);
    } else {
        output_append(output, "\r" CLEAR_LINE
            DIM "%s %s is now known as %s" RESET "\r\n",
            timestamp, event->nickname, event->message);
    }

}

void protocol_join(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    struct kirc_context *ctx = event->ctx;
    const char *timestamp = protocol_get_time();

    if (strcmp(event->nickname, ctx->nickname) == 0) {
        output_append(output, "\r" CLEAR_LINE
            DIM "%s you joined %s" RESET "\r\n",
            timestamp, event->channel);
    } else {
        protocol_noop(network, event, output);
    }
}

void protocol_part(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    struct kirc_context *ctx = event->ctx;
    const char *timestamp = protocol_get_time();

    if (strcmp(event->nickname, ctx->nickname) == 0) {
        output_append(output, "\r" CLEAR_LINE
            DIM "%s you left %s" RESET "\r\n",
            timestamp, event->channel);
    } else {
        protocol_noop(network, event, output);
    }
}

/**
 * protocol_ctcp_action() - Display CTCP ACTION message
 * @network: Network connection (unused)
 * @event: Event containing ACTION details
 * @output: Output buffer for display
 *
 * Displays a CTCP ACTION message (/me command) with a bullet point
 * prefix, timestamp, nickname, and action text in dimmed format.
 */
void protocol_ctcp_action(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    output_append(output, "\r" CLEAR_LINE DIM "%s \u2022 %s %s" RESET "\r\n",
        protocol_get_time(), event->nickname, event->message);
}

/**
 * protocol_ctcp_info() - Display CTCP informational message
 * @network: Network connection (unused)
 * @event: Event containing CTCP details
 * @output: Output buffer for display
 *
 * Displays CTCP protocol messages (CLIENTINFO, DCC, PING, TIME, VERSION)
 * with sender's nickname in bold blue and appropriate label. Shows
 * parameters or message content if present.
 */
void protocol_ctcp_info(struct network *network, struct event *event, struct output *output)
{
    (void)network;

    const char *label = "";
    const char *timestamp = protocol_get_time();

    switch(event->type) {
    case EVENT_CTCP_CLIENTINFO:
        label = "CLIENTINFO";
        break;

    case EVENT_CTCP_DCC:
        label = "DCC";
        break;

    case EVENT_CTCP_PING:
        label = "PING";
        break;

    case EVENT_CTCP_TIME:
        label = "TIME";
        break;

    case EVENT_CTCP_VERSION: 
        label = "VERSION";
        break;

    default:
        label = "CTCP";
        break;
    }

    if (event->params[0] != '\0') {
        output_append(output, "\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            timestamp, event->nickname, label, event->params);
    } else if (event->message[0] != '\0') {
        output_append(output, "\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            timestamp, event->nickname, label, event->message);
    } else {
        output_append(output, "\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s\r\n",
            timestamp, event->nickname, label);
    }
}
