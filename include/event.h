#ifndef __KIRC_EVENT_H
#define __KIRC_EVENT_H

#include "kirc.h"

typedef enum {
    EVENT_NONE = 0,
    EVENT_CAP,
    EVENT_JOIN,
    EVENT_MODE,
    EVENT_NICK,
    EVENT_NOTICE,
    EVENT_PART,
    EVENT_PING,
    EVENT_PRIVMSG,
    EVENT_QUIT,
    EVENT_001_RPL_WELCOME,
    EVENT_002_RPL_YOURHOST,
    EVENT_003_RPL_CREATED,
    EVENT_004_RPL_MYINFO,
    EVENT_005_RPL_BOUNCE,
    EVENT_250_RPL_STATSCONN,
    EVENT_251_RPL_LUSERCLIENT,
    EVENT_252_RPL_LUSEROP,
    EVENT_254_RPL_LUSERUNKNOWN,
    EVENT_253_RPL_LUSERCHANNELS,
    EVENT_255_RPL_LUSERME,
    EVENT_265_RPL_LOCALUSERS,
    EVENT_266_RPL_GLOBALUSERS,
    EVENT_328_RPL_CHANNEL_URL,
    EVENT_332_RPL_TOPIC,
    EVENT_333_RPL_TOPICWHOTIME,
    EVENT_353_RPL_NAMREPLY,
    EVENT_366_RPL_ENDOFNAMES,
    EVENT_372_RPL_MOTD,
    EVENT_375_RPL_MOTDSTART,
    EVENT_376_RPL_ENDOFMOTD,
    EVENT_904_ERR_SASLFAIL,
    EVENT_908_RPL_SASLMECHS
} event_type_t;

typedef struct {
    const char *command;
    event_type_t type;
} event_map_t;

static const event_map_t event_map[] = {
    { "CAP",     EVENT_CAP },
    { "JOIN",    EVENT_JOIN },
    { "MODE",    EVENT_MODE },
    { "NICK",    EVENT_NICK },
    { "NOTICE",  EVENT_NOTICE },
    { "PART",    EVENT_PART },
    { "PING",    EVENT_PING },
    { "PRIVMSG", EVENT_PRIVMSG },
    { "QUIT",    EVENT_QUIT },
    { "001",     EVENT_001_RPL_WELCOME },
    { "002",     EVENT_002_RPL_YOURHOST },
    { "003",     EVENT_003_RPL_CREATED },
    { "004",     EVENT_004_RPL_MYINFO },
    { "005",     EVENT_005_RPL_BOUNCE },
    { "250",     EVENT_250_RPL_STATSCONN },
    { "251",     EVENT_251_RPL_LUSERCLIENT },
    { "252",     EVENT_252_RPL_LUSEROP },
    { "253",     EVENT_253_RPL_LUSERUNKNOWN },
    { "254",     EVENT_254_RPL_LUSERCHANNELS },
    { "255",     EVENT_255_RPL_LUSERME },
    { "265",     EVENT_265_RPL_LOCALUSERS },
    { "266",     EVENT_266_RPL_GLOBALUSERS },
    { "328",     EVENT_328_RPL_CHANNEL_URL },
    { "332",     EVENT_332_RPL_TOPIC },
    { "333",     EVENT_333_RPL_TOPICWHOTIME },
    { "353",     EVENT_353_RPL_NAMREPLY },
    { "366",     EVENT_366_RPL_ENDOFNAMES },
    { "375",     EVENT_375_RPL_MOTDSTART },
    { "372",     EVENT_372_RPL_MOTD },
    { "376",     EVENT_376_RPL_ENDOFMOTD },
    { "904",     EVENT_904_ERR_SASLFAIL },
    { "908",     EVENT_908_RPL_SASLMECHS },
    { NULL,      EVENT_NONE }
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
