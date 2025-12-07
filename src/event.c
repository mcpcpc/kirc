#include "event.h"

int event_init(event_t *event, char *line)
{

    memset(event, 0, sizeof(*event));

    if (strncmp(line, "PING", 4) == 0) {
        event->type = EVENT_PING;
        return 0;
    }

    char *prefix = strtok(line, " ") + 1;
    char *suffix = strtok(NULL, ":");
    char *message = strtok(NULL, "\r");
    char *nickname = strtok(prefix, "!");
    char *command = strtok(suffix, "#& ");
    char *channel = strtok(NULL, " \r");
    char *params = strtok(NULL, ":\r");

    size_t message_n = sizeof(event->message) - 1;
    strncpy(event->message, message, message_n);

    size_t nickname_n = sizeof(event->nickname) - 1;
    strncpy(event->nickname, nickname, nickname_n);

    size_t channel_n = sizeof(event->channel) - 1;
    strncpy(event->channel, channel, channel_n);
    printf("test!\n");
    size_t command_n = sizeof(event->command) - 1;
    strncpy(event->command, command, command_n);

    if (params[0] != '\0') {
        size_t params_n = sizeof(event->params) - 1;
        strncpy(event->params, params, params_n);
    }

    if (!strncmp(command, "001", 3)) {
        event->type = EVENT_JOIN;
        return 0;
    } else if (!strncmp(command, "QUIT", 4)) {
        event->type = EVENT_QUIT;
        return 0;
    } else if (!strncmp(command, "PART", 4)) {
        event->type = EVENT_PART;
        return 0;
    } else if (!strncmp(command, "JOIN", 4)) {
        event->type = EVENT_JOIN;
        return 0;
    } else if (!strncmp(command, "NICK", 4)) {
        event->type = EVENT_NICK;
        return 0;
    } else if (!strncmp(command, "PRIVMSG", 7)) {
        event->type = EVENT_PRIVMSG;
        return 0;
    } else {
        return 1;
    }

    return 0;
}
