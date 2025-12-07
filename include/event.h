#ifndef __KIRC_EVENT_H
#define __KIRC_EVENT_H

#include "kirc.h"

typedef enum {
    EVENT_NONE = 0,
    EVENT_PING,
    EVENT_PRIVMSG,
    EVENT_JOIN,
    EVENT_PART,
    EVENT_QUIT,
    EVENT_NICK,
    EVENT_NUMERIC,
    EVENT_RAW
} event_type_t;

typedef struct {
    event_type_t type;
    char channel[RFC1459_CHANNEL_MAX_LEN];
    char message[RFC1459_MESSAGE_MAX_LEN];
    char command[RFC1459_MESSAGE_MAX_LEN];
    char nickname[RFC1459_MESSAGE_MAX_LEN];
    char params[RFC1459_MESSAGE_MAX_LEN];
} event_t;

int event_init(event_t *ev, char *line);

#endif  // __KIRC_EVENT_H
