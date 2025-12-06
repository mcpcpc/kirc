#ifndef __KIRC_EVENT_H
#define __KIRC_EVENT_H

#include "kirc.h"

typedef enum {
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
    char *nick;
    char *user;
    char *host;
} identity_t;

typedef struct {
    event_type_t type;

    identity_t origin;

    char *command;
    char *channel;
    char *message;
    char *params;

    int numeric;
} event_t;

#endif  // __KIRC_EVENT_H
