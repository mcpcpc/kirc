/*
 * protocol.c
 * IRC protocol handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "protocol.h"

static void protocol_get_time(char *out)
{
    time_t current;
    time(&current);
    struct tm *info = localtime(&current);
    strftime(out, KIRC_TIMESTAMP_SIZE,
        KIRC_TIMESTAMP_FORMAT, info);
}

void protocol_noop(struct network *network, struct event *event)
{
    (void)network;
    (void)event;
}

void protocol_ping(struct network *network, struct event *event)
{
    network_send(network, "PONG :%s\r\n", event->message);
}

void protocol_authenticate(struct network *network, struct event *event)
{
    (void)event;

    network_authenticate(network);
}

void protocol_welcome(struct network *network, struct event *event)
{
    (void)event;

    for (int i = 0; network->ctx->channels[i][0] != '\0'; ++i) {
        network_send(network, "JOIN %s\r\n",
            network->ctx->channels[i]);
    }
}

void protocol_raw(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " REVERSE "%s" RESET "\r\n",
        hhmm, event->raw);
}

void protocol_info(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s %s" RESET "\r\n",
        hhmm, event->message);
}

void protocol_error(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_RED "%s" RESET "\r\n",
        hhmm, event->message);
}

void protocol_notice(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " %s\r\n",
        hhmm, event->nickname, event->message);
}

static void protocol_privmsg_direct(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " " BLUE "%s" RESET "\r\n",
        hhmm, event->nickname, event->message);
}

static void protocol_privmsg_indirect(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD "%s" RESET " [%s]: %s\r\n",
        hhmm, event->nickname, event->channel, event->message);

}

void protocol_privmsg(struct network *network, struct event *event)
{
    char *channel = event->channel;
    char *nickname = event->ctx->nickname;

    if (strcmp(channel, nickname) == 0) {
        protocol_privmsg_direct(network, event);
    } else {
        protocol_privmsg_indirect(network, event);
    }
}

void protocol_nick(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    struct kirc_context *ctx = event->ctx;
    const char *nickname = event->nickname;
    const char *message = event->message;
    
    if (ctx && strcmp(nickname, ctx->nickname) == 0) {
        size_t siz = sizeof(ctx->nickname) - 1;
        strncpy(ctx->nickname, message, siz);
        ctx->nickname[siz] = '\0';
        printf("\r" CLEAR_LINE
            DIM "%s you are now known as %s" RESET "\r\n",
            hhmm, message);
    } else {
        printf("\r" CLEAR_LINE
            DIM "%s %s is now known as %s" RESET "\r\n",
            hhmm, nickname, message);
    }

}

void protocol_ctcp_action(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s \u2022 %s %s" RESET "\r\n",
        hhmm, event->nickname, event->message);
}

void protocol_ctcp_info(struct network *network, struct event *event)
{
    (void)network;

    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    const char *label = "";

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
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            hhmm, event->nickname, event->label,
            event->params);
    } else if (event->message[0] != '\0') {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            hhmm, event->nickname, event->label,
            event->message);
    } else {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s\r\n",
            hhmm, event->nickname, event->label);
    }
}
