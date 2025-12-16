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
        printf("\r\x1b[0K\x1b[2m%s\x1b[0m \x1b[1;31m%s\x1b[0m %s\r\n",
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
    case EVENT_NUMERIC_RPL_WELCOME:
    case EVENT_NUMERIC_RPL_YOURHOST:
    case EVENT_NUMERIC_RPL_CREATED:
    case EVENT_NUMERIC_RPL_MYINFO:
    case EVENT_NUMERIC_RPL_BOUNCE:
    case EVENT_NUMERIC_RPL_STATSCONN:
    case EVENT_NUMERIC_RPL_LUSERCLIENT:
    case EVENT_NUMERIC_RPL_LUSEROP:
    case EVENT_NUMERIC_RPL_LUSERUNKNOWN:
    case EVENT_NUMERIC_RPL_LUSERCHANNELS:
    case EVENT_NUMERIC_RPL_LUSERME:
    case EVENT_NUMERIC_RPL_LOCALUSERS:
    case EVENT_NUMERIC_RPL_GLOBALUSERS:
    case EVENT_NUMERIC_RPL_NAMREPLY:
    case EVENT_NUMERIC_RPL_MOTDSTART:
    case EVENT_NUMERIC_RPL_MOTD:
    case EVENT_NUMERIC_RPL_ENDOFMOTD:
    case EVENT_NUMERIC_RPL_SASLMECHS:
        feed_info(event);
        break;

    case EVENT_NUMERIC_ERR_SASLFAIL:
        feed_error(event);
        break;

    default:
        feed_channel(event);
        break;
    }
}
