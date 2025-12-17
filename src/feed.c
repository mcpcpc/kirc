#include "feed.h"

static void get_time(char *hhmmss)
{
    time_t current;
    time(&current);
    struct tm *info = localtime(&current);
    strftime(hhmmss, 6, "%H:%M", info);
}

static void feed_info(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    printf("\r\x1b[0K\x1b[2m%s %s\x1b[0m\r\n",
        hhmmss, event->message);
}

static void feed_error(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    printf("\r\x1b[0K\x1b[2m%s \x1b[1;31m%s\x1b[0m\r\n",
        hhmmss, event->message);
}

static void feed_notice(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1;34m%s\x1b[0m %s\r\n",
        hhmmss, event->nickname, event->message);
}

static void feed_privmsg(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    if (strcmp(event->channel, event->ctx->nickname) == 0) {
        printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1;34m%s\x1b[0m \x1b[34m%s\x1b[0m\r\n",
            hhmmss, event->nickname, event->message);
    } else {
        printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1m%s\x1b[0m %s\r\n",
            hhmmss, event->nickname, event->message);
    }
}

static void feed_join(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    printf("\r\x1b[0K\x1b[2m--->> %s [%s]\x1b[0m\r\n",
        event->nickname, event->channel);
}

static void feed_part(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    printf("\r\x1b[0K\x1b[2m<<--- %s\x1b[0m\r\n",
        event->nickname);
}

static void feed_quit(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    printf("\r\x1b[0K\x1b[2m<<<<< %s\x1b[0m\r\n",
        event->nickname);
}

static void feed_nick(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[2m%s --> %s\x1b[0m\r\n",
        hhmmss, event->nickname, event->message);
}

static void feed_channel(event_t *event)
{
    char hhmmss[6];
    get_time(hhmmss);

    if (event->channel[0] != '\0') {
        printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1m%s\x1b[0m %s\r\n",
            hhmmss, event->channel, event->message);
    } else {
        printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1m%s\x1b[0m %s\r\n",
            hhmmss, event->nickname, event->message);
    }
}

void feed_render(event_t *event)
{
    if (event->ctx->filtered) {
        if (strcmp(event->channel, event->ctx->selected) != 0) {
            return;
        }
    }
    
    switch (event->type) {
    case EVENT_PING:
        break;

    case EVENT_PRIVMSG:
        feed_privmsg(event);
        break;

    case EVENT_JOIN:
        feed_join(event);
        break;

    case EVENT_PART:
        feed_part(event);
        break;

    case EVENT_QUIT:
        feed_quit(event);
        break;

    case EVENT_NICK:
        feed_nick(event);
        break;

    case EVENT_NOTICE:
        feed_notice(event);
        break;

    case EVENT_CAP:
    case EVENT_MODE:
    case EVENT_001_RPL_WELCOME:
    case EVENT_002_RPL_YOURHOST:
    case EVENT_003_RPL_CREATED:
    case EVENT_004_RPL_MYINFO:
    case EVENT_005_RPL_BOUNCE:
    case EVENT_200_RPL_TRACELINK:
    case EVENT_201_RPL_TRACECONNECTING:
    case EVENT_202_RPL_TRACEHANDSHAKE:
    case EVENT_203_RPL_TRACEUNKNOWN:
    case EVENT_204_RPL_TRACEOPERATOR:
    case EVENT_205_RPL_TRACEUSER:
    case EVENT_206_RPL_TRACESERVER:
    case EVENT_207_RPL_TRACESERVICE:
    case EVENT_208_RPL_TRACENEWTYPE:
    case EVENT_209_RPL_TRACECLASS:
    case EVENT_250_RPL_STATSCONN:
    case EVENT_251_RPL_LUSERCLIENT:
    case EVENT_252_RPL_LUSEROP:
    case EVENT_253_RPL_LUSERUNKNOWN:
    case EVENT_254_RPL_LUSERCHANNELS:
    case EVENT_255_RPL_LUSERME:
    case EVENT_265_RPL_LOCALUSERS:
    case EVENT_266_RPL_GLOBALUSERS:
    case EVENT_328_RPL_CHANNEL_URL:
    case EVENT_332_RPL_TOPIC:
    case EVENT_333_RPL_TOPICWHOTIME:
    case EVENT_353_RPL_NAMREPLY:
    case EVENT_366_RPL_ENDOFNAMES:
    case EVENT_372_RPL_MOTD:
    case EVENT_375_RPL_MOTDSTART:
    case EVENT_376_RPL_ENDOFMOTD:
    case EVENT_908_RPL_SASLMECHS:
        feed_info(event);
        break;

    case EVENT_400_ERR_UNKNOWNERROR:
    case EVENT_401_ERR_NOSUCHNICK:
    case EVENT_402_ERR_NOSUCHSERVER:
    case EVENT_403_ERR_NOSUCHCHANNEL:
    case EVENT_404_ERR_CANNOTSENDTOCHAN:
    case EVENT_405_ERR_TOOMANYCHANNELS:
    case EVENT_406_ERR_WASNOSUCHNICK:
    case EVENT_407_ERR_TOOMANYTARGETS:
    case EVENT_408_ERR_NOSUCHSERVICE:
    case EVENT_409_ERR_NOORIGIN:
    case EVENT_411_ERR_NORECIPIENT:
    case EVENT_412_ERR_NOTEXTTOSEND:
    case EVENT_413_ERR_NOTOPLEVEL:
    case EVENT_414_ERR_WILDTOPLEVEL:
    case EVENT_415_ERR_BADMASK:
    case EVENT_421_ERR_UNKNOWNCOMMAND:
    case EVENT_422_ERR_NOMOTD:
    case EVENT_423_ERR_NOADMININFO:
    case EVENT_424_ERR_FILEERROR:
    case EVENT_904_ERR_SASLFAIL:
        feed_error(event);
        break;

    default:
        feed_channel(event);
        break;
    }
}
