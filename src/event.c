#include "event.h"

int event_init(event_t *event, char *line)
{

    memset(event, 0, sizeof(*event));

    if (strncmp(line, "PING", 4) == 0) {
        event->type = EVENT_PING;
        return 0;
    }

    char *prefix, *suffix, *message, *nickname;
    char *command, *channel, *params;

    prefix = strtok(line, " ") + 1;
    suffix = strtok(NULL, ":");
    message = strtok(NULL, "\r");
    nickname = strtok(prefix, "!");
    command = strtok(suffix, "#& ");
    channel = strtok(NULL, " \r");
    params = strtok(NULL, ":\r");

    size_t message_n = sizeof(event->message) - 1;
    strncpy(event->message, message, message_n);

    size_t channel_n = sizeof(event->channel) - 1;
    strncpy(event->channel, channel, channel_n);

    size_t command_n = sizeof(event->command) - 1;
    strncpy(event->command, command, command_n);

    size_t params_n = sizeof(event->params) - 1;
    strncpy(event->params, params, params_n);

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
