/*
 * protocol.c
 * IRC protocol handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "protocol.h"

void protocol_get_time(char *out)
{
    time_t current;
    time(&current);
    struct tm *info = localtime(&current);
    strftime(out, KIRC_TIMESTAMP_SIZE,
        KIRC_TIMESTAMP_FORMAT, info);
}

void protocol_noop(protocol_t *protocol)
{
    (void)protocol;  /* no operation */
}

void protocol_raw(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " REVERSE "%s" RESET "\r\n",
        hhmm, protocol->raw);
}

void protocol_info(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s %s" RESET "\r\n",
        hhmm, protocol->message);
}

void protocol_error(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_RED "%s" RESET "\r\n",
        hhmm, protocol->message);
}

void protocol_notice(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " %s\r\n",
        hhmm, protocol->nickname, protocol->message);
}

void protocol_privmsg_direct(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD_BLUE "%s" RESET " " BLUE "%s" RESET "\r\n",
        hhmm, protocol->nickname, protocol->message);
}

void protocol_privmsg_indirect(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s" RESET
        " " BOLD "%s" RESET " [%s]: %s\r\n",
        hhmm, protocol->nickname, protocol->channel,
        protocol->message);

}

void protocol_privmsg(protocol_t *protocol)
{
    char *channel = protocol->channel;
    char *nickname = protocol->ctx->nickname;

    if (strcmp(channel, nickname) == 0) {
        protocol_privmsg_direct(protocol);
    } else {
        protocol_privmsg_indirect(protocol);
    }
}

void protocol_nick(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    
    if (strcmp(protocol->nickname, protocol->ctx->nickname) == 0) {
        size_t siz = sizeof(protocol->ctx->nickname) - 1;
        strncpy(protocol->ctx->nickname, protocol->message, siz);
        protocol->ctx->nickname[siz] = '\0';
        printf("\r" CLEAR_LINE
            DIM "%s you are now known as %s" RESET "\r\n",
            hhmm, protocol->message);
    } else {
        printf("\r" CLEAR_LINE
            DIM "%s %s is now known as %s" RESET "\r\n",
            hhmm, protocol->nickname, protocol->message);
    }

}

void protocol_ctcp_action(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    printf("\r" CLEAR_LINE DIM "%s * %s %s" RESET "\r\n",
        hhmm, protocol->nickname, protocol->message);
}

void protocol_ctcp_info(protocol_t *protocol)
{
    char hhmm[KIRC_TIMESTAMP_SIZE];
    protocol_get_time(hhmm);

    const char *label = "";

    switch(protocol->event) {
    case PROTOCOL_EVENT_CTCP_CLIENTINFO:
        label = "CLIENTINFO";
        break;

    case PROTOCOL_EVENT_CTCP_DCC:
        label = "DCC";
        break;

    case PROTOCOL_EVENT_CTCP_PING:
        label = "PING";
        break;

    case PROTOCOL_EVENT_CTCP_TIME:
        label = "TIME";
        break;

    case PROTOCOL_EVENT_CTCP_VERSION: 
        label = "VERSION";
        break;

    default:
        label = "CTCP";
        break;
    }

    if (protocol->params[0] != '\0') {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            hhmm, protocol->nickname, label,
            protocol->params);
    } else if (protocol->message[0] != '\0') {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s: %s\r\n",
            hhmm, protocol->nickname, label,
            protocol->message);
    } else {
        printf("\r" CLEAR_LINE DIM "%s " RESET
            BOLD_BLUE "%s" RESET " %s\r\n",
            hhmm, protocol->nickname, label);
    }
}

int protocol_ctcp_parse(protocol_t *protocol)
{
    if (((protocol->event == PROTOCOL_EVENT_PRIVMSG) ||
        (protocol->event == PROTOCOL_EVENT_NOTICE)) &&
        (protocol->message[0] == '\001')) {
        char ctcp[MESSAGE_MAX_LEN];
        size_t siz = sizeof(protocol->message);
        size_t len = strnlen(protocol->message, siz);
        size_t end = 1;

        while ((end < len) && protocol->message[end] != '\001') {
            end++;
        }

        size_t ctcp_len = (end > 1) ? end - 1 : 0;

        if (ctcp_len > 0) {
            size_t copy_n;

            if (ctcp_len < sizeof(ctcp)) {
                copy_n = ctcp_len + 1;
            } else {
                copy_n = sizeof(ctcp);
            }

            safecpy(ctcp, protocol->message + 1, copy_n); 

            char *command = strtok(ctcp, " ");
            char *args = strtok(NULL, "");

            if (command != NULL) {
                if (strcmp(command, "ACTION") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_ACTION;
                    if (args != NULL) {
                        siz = sizeof(protocol->message);
                        safecpy(protocol->message, args, siz);
                    } else {
                        protocol->message[0] = '\0';
                    }
                } else if (strcmp(command, "VERSION") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_VERSION;
                    if (args != NULL) {
                        siz = sizeof(protocol->params);
                        safecpy(protocol->params, args, siz);
                    } else {
                        protocol->params[0] = '\0';
                    }
                } else if (strcmp(command, "PING") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_PING;
                    if (args != NULL) {
                        siz = sizeof(protocol->message);
                        safecpy(protocol->message, args, siz);
                    } else {
                        protocol->message[0] = '\0';
                    }
                } else if (strcmp(command, "TIME") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_TIME;
                } else if (strcmp(command, "CLIENTINFO") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_CLIENTINFO;
                } else if (strcmp(command, "DCC") == 0) {
                    protocol->event = PROTOCOL_EVENT_CTCP_DCC;
                    if (args != NULL) {
                        siz = sizeof(protocol->message);
                        safecpy(protocol->message, args, siz);
                    } else {
                        protocol->message[0] = '\0';
                    }
                } else {
                    protocol->message[0] = '\0';
                }
            }
        }
    }

    return 0;
}

int protocol_init(protocol_t *protocol, kirc_t *ctx)
{
    memset(protocol, 0, sizeof(*protocol));

    protocol->ctx = ctx;
    protocol->event = PROTOCOL_EVENT_NONE;

    return 0;
}

int protocol_parse(protocol_t *protocol, char *line)
{

    size_t raw_n = sizeof(protocol->raw);
    safecpy(protocol->raw, line, raw_n);

    if (strncmp(line, "PING", 4) == 0) {
        protocol->event = PROTOCOL_EVENT_PING;
        size_t message_n = sizeof(protocol->message);
        safecpy(protocol->message, line + 6, message_n);
        return 0;
    }

    if (strncmp(line, "AUTHENTICATE +", 14) == 0) {
        protocol->event = PROTOCOL_EVENT_EXT_AUTHENTICATE;
        return 0;
    }

    if (strncmp(line, "ERROR", 5) == 0) {
        protocol->event = PROTOCOL_EVENT_ERROR;
        size_t message_n = sizeof(protocol->message);
        safecpy(protocol->message, line + 7, message_n);
        return 0;
    }

    char *token = strtok(line, " ");
    if (token == NULL) return -1;
    char *prefix = token + 1;
    char *suffix = strtok(NULL, ":");
    char *message = strtok(NULL, "\r");

    if (message != NULL) {
        size_t message_n = sizeof(protocol->message);
        safecpy(protocol->message, message, message_n);
    }

    char *nickname = strtok(prefix, "!");

    if (nickname != NULL) {
        size_t nickname_n = sizeof(protocol->nickname);
        safecpy(protocol->nickname, nickname, nickname_n);
    }

    char *command = strtok(suffix, "#& ");

    if (command != NULL) {
        size_t command_n = sizeof(protocol->command);
        safecpy(protocol->command, command, command_n);
    }

    char *channel = strtok(NULL, " \r");

    if (channel != NULL) {
        size_t channel_n = sizeof(protocol->channel);
        safecpy(protocol->channel, channel, channel_n);
    }

    char *params = strtok(NULL, ":\r");

    if (params != NULL) {
        size_t params_n = sizeof(protocol->params);
        safecpy(protocol->params, params, params_n);
    }

    for (int i = 0; event_table[i].command != NULL; i++) {
        if (strcmp(protocol->command, event_table[i].command) == 0) {
            protocol->event = event_table[i].event;
            break;
        }
    }

    protocol_ctcp_parse(protocol);

    return 0;
}

int protocol_handle(protocol_t *protocol)
{
    for(int i = 0; dispatch_table[i].handler != NULL; ++i) {
        if (dispatch_table[i].event == protocol->event) {
            dispatch_table[i].handler(protocol);
            return 0;
        }
    }

    return -1;
}
