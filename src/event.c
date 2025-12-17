#include "event.h"

int event_init(event_t *event, kirc_t *ctx, char *line)
{

    memset(event, 0, sizeof(*event));

    event->ctx = ctx;

    size_t raw_n = sizeof(event->raw) - 1;
    strncpy(event->raw, line, raw_n);

    if (strncmp(line, "PING", 4) == 0) {
        event->type = EVENT_PING;
        return 0;
    }

    char *prefix = strtok(line, " ") + 1;
    char *suffix = strtok(NULL, ":");
    char *message = strtok(NULL, "\r");

    if (message != NULL) {
        size_t message_n = sizeof(event->message) - 1;
        strncpy(event->message, message, message_n);
    }

    char *nickname = strtok(prefix, "!");

    if (nickname != NULL) {
        size_t nickname_n = sizeof(event->nickname) - 1;
        strncpy(event->nickname, nickname, nickname_n);
    }

    char *command = strtok(suffix, "#& ");

    if (command != NULL) {
        size_t command_n = sizeof(event->command) - 1;
        strncpy(event->command, command, command_n);
    }

    char *channel = strtok(NULL, " \r");

    if (channel != NULL) {
        size_t channel_n = sizeof(event->channel) - 1;
        strncpy(event->channel, channel, channel_n);
    }

    char *params = strtok(NULL, ":\r");

    if (params != NULL) {
        size_t params_n = sizeof(event->params) - 1;
        strncpy(event->params, params, params_n);
    }

    for (int i = 0; event_map[i].command != NULL; i++) {
        if (strcmp(event->command, event_map[i].command) == 0) {
            event->type = event_map[i].type;
            break;
        }
    }

    return 0;
}
