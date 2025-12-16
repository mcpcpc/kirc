#ifndef __KIRC_EVENT_H
#define __KIRC_EVENT_H

#include "kirc.h"

typedef enum {
    EVENT_NONE = 0,
    EVENT_JOIN,
    EVENT_PART,
    EVENT_PING,
    EVENT_PRIVMSG,
    EVENT_NICK,
    EVENT_NOTICE,
    EVENT_NUMERIC_RPL_WELCOME,
    EVENT_NUMERIC_RPL_YOURHOST,
    EVENT_NUMERIC_RPL_CREATED,
    EVENT_NUMERIC_RPL_MYINFO,
    EVENT_NUMERIC_RPL_BOUNCE,
    EVENT_NUMERIC_RPL_STATSCONN,
    EVENT_NUMERIC_RPL_LUSERCLIENT,
    EVENT_NUMERIC_RPL_LUSEROP,
    EVENT_NUMERIC_RPL_LUSERCHANNELS,
    EVENT_NUMERIC_RPL_LUSERME,
    EVENT_NUMERIC_RPL_LUSERUNKNOWN,
    EVENT_NUMERIC_RPL_LOCALUSERS,
    EVENT_NUMERIC_RPL_GLOBALUSERS,
    EVENT_NUMERIC_RPL_MOTD,
    EVENT_NUMERIC_RPL_MOTDSTART,
    EVENT_NUMERIC_RPL_ENDOFMOTD,
    EVENT_QUIT
} event_type_t;

typedef struct {
    const char *command;
    event_type_t type;
} event_map_t;

static const event_map_t event_map[] = {
    { "JOIN",    EVENT_JOIN },
    { "PART",    EVENT_PART },
    { "PING",    EVENT_PING },
    { "PRIVMSG", EVENT_PRIVMSG },
    { "NICK",    EVENT_NICK },
    { "NOTICE",  EVENT_NOTICE },
    { "001",     EVENT_NUMERIC_RPL_WELCOME },
    { "002",     EVENT_NUMERIC_RPL_YOURHOST },
    { "003",     EVENT_NUMERIC_RPL_CREATED },
    { "004",     EVENT_NUMERIC_RPL_MYINFO },
    { "005",     EVENT_NUMERIC_RPL_BOUNCE },
    { "250",     EVENT_NUMERIC_RPL_STATSCONN },
    { "251",     EVENT_NUMERIC_RPL_LUSERCLIENT },
    { "252",     EVENT_NUMERIC_RPL_LUSEROP },
    { "253",     EVENT_NUMERIC_RPL_LUSERUNKNOWN },
    { "254",     EVENT_NUMERIC_RPL_LUSERCHANNELS },
    { "255",     EVENT_NUMERIC_RPL_LUSERME },
    { "265",     EVENT_NUMERIC_RPL_LOCALUSERS },
    { "266",     EVENT_NUMERIC_RPL_GLOBALUSERS },
    { "375",     EVENT_NUMERIC_RPL_MOTDSTART },
    { "372",     EVENT_NUMERIC_RPL_MOTD },
    { "376",     EVENT_NUMERIC_RPL_ENDOFMOTD },
    { "QUIT",    EVENT_QUIT },
    { NULL, EVENT_NONE }
};

typedef struct {
    kirc_t *ctx;
    char channel[RFC1459_CHANNEL_MAX_LEN];
    char message[RFC1459_MESSAGE_MAX_LEN];
    char command[RFC1459_MESSAGE_MAX_LEN];
    char nickname[RFC1459_MESSAGE_MAX_LEN];
    char params[RFC1459_MESSAGE_MAX_LEN];
    event_type_t type;
} event_t;

int event_init(event_t *ev, kirc_t *ctx, char *line);

#endif  // __KIRC_EVENT_H
