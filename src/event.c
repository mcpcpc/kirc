/*
 * event.c
 * Message event handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "event.h"

int event_init(struct event *event, struct kirc_context *ctx)
{
    memset(event, 0, sizeof(*event));

    event->ctx = ctx;
    event->type = EVENT_NONE;

    return 0;
}

static int event_ctcp_parse(struct event *event)
{
    if (((event->type == EVENT_PRIVMSG) ||
        (event->type == EVENT_NOTICE)) &&
        (event->message[0] == '\001')) {
        char ctcp[MESSAGE_MAX_LEN];
        size_t siz = sizeof(event->message);
        size_t len = strnlen(event->message, siz);
        size_t end = 1;

        while ((end < len) && event->message[end] != '\001') {
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

            safecpy(ctcp, event->message + 1, copy_n); 

            char *command = strtok(ctcp, " ");
            char *args = strtok(NULL, "");

            if (command != NULL) {
                if (strcmp(command, "ACTION") == 0) {
                    event->type = EVENT_CTCP_ACTION;
                    if (args != NULL) {
                        siz = sizeof(event->message);
                        safecpy(event->message, args, siz);
                    } else {
                        event->message[0] = '\0';
                    }
                } else if (strcmp(command, "VERSION") == 0) {
                    event->type = EVENT_CTCP_VERSION;
                    if (args != NULL) {
                        siz = sizeof(event->params);
                        safecpy(event->params, args, siz);
                    } else {
                        event->params[0] = '\0';
                    }
                } else if (strcmp(command, "PING") == 0) {
                    event->type = EVENT_CTCP_PING;
                    if (args != NULL) {
                        siz = sizeof(event->message);
                        safecpy(event->message, args, siz);
                    } else {
                        event->message[0] = '\0';
                    }
                } else if (strcmp(command, "TIME") == 0) {
                    event->type = EVENT_CTCP_TIME;
                } else if (strcmp(command, "CLIENTINFO") == 0) {
                    event->type = EVENT_CTCP_CLIENTINFO;
                } else if (strcmp(command, "DCC") == 0) {
                    event->type = EVENT_CTCP_DCC;
                    if (args != NULL) {
                        siz = sizeof(event->message);
                        safecpy(event->message, args, siz);
                    } else {
                        event->message[0] = '\0';
                    }
                } else {
                    event->message[0] = '\0';
                }
            }
        }
    }

    return 0;
}

int event_parse(struct event *event, char *line)
{

    char line_copy[MESSAGE_MAX_LEN];
    safecpy(line_copy, line, sizeof(line_copy));
    safecpy(event->raw, line, sizeof(event->raw));

    if (strncmp(line, "PING", 4) == 0) {
        event->type = EVENT_PING;
        size_t message_n = sizeof(event->message);
        safecpy(event->message, line + 6, message_n);
        return 0;
    }

    if (strncmp(line, "AUTHENTICATE +", 14) == 0) {
        event->type = EVENT_EXT_AUTHENTICATE;
        return 0;
    }

    if (strncmp(line, "ERROR", 5) == 0) {
        event->type = EVENT_ERROR;
        size_t message_n = sizeof(event->message);
        safecpy(event->message, line + 7, message_n);
        return 0;
    }

    char *token = strtok(line_copy, " ");

    if (token == NULL) {
        return -1;
    }

    char *prefix = token + 1;
    char *suffix = strtok(NULL, ":");

    if (suffix == NULL) {
        return -1;
    }

    char *message = strtok(NULL, "\r");

    if (message != NULL) {
        size_t message_n = sizeof(event->message);
        safecpy(event->message, message, message_n);
    }

    char *nickname = strtok(prefix, "!");

    if (nickname != NULL) {
        size_t nickname_n = sizeof(event->nickname);
        safecpy(event->nickname, nickname, nickname_n);
    }

    char *command = strtok(suffix, "#& ");

    if (command != NULL) {
        size_t command_n = sizeof(event->command);
        safecpy(event->command, command, command_n);
    }

    char *channel = strtok(NULL, " \r");

    if (channel != NULL) {
        size_t channel_n = sizeof(event->channel);
        safecpy(event->channel, channel, channel_n);
    }

    char *params = strtok(NULL, ":\r");

    if (params != NULL) {
        size_t params_n = sizeof(event->params);
        safecpy(event->params, params, params_n);
    }

    for (int i = 0; event_table[i].command != NULL; i++) {
        if (strcmp(event->command, event_table[i].command) == 0) {
            event->type = event_table[i].type;
            break;
        }
    }

    event_ctcp_parse(event);

    return 0;
}
