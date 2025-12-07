#include "feed.h"

static void feed_wordwrap(char *message, int cols)
{
}

static void feed_privmsg(event_t *event)
{
    int cols = terminal_columns(event->ctx);

    printf("%s: ", event->nickname);
    feed_wordwrap(event->message, cols);
}

static void feed_join(event_t *event)
{
    printf("--> %s", event->nickname);
}

static void feed_part(event_t *event)
{
    printf("<-- %s", event->nickname);
}

static void feed_quit(event_t *event)
{
    printf("<<< %s", event->nickname);
}

static void feed_nick(event_t *event)
{
    printf("%s --> %s", event->nickname, event->message);
}

static void feed_channel(event_t *event)
{
    if (event->channel != NULL) {
        printf("%s: ", event->channel);
    } else {
        printf("%s: ", event->nickname);
    }
}

void feed_render(event_t *event)
{
    if (event->type == EVENT_PRIVMSG) {
        feed_privmsg(event);
    } else if (event->type == EVENT_JOIN) {
        feed_join(event);
    } else if (event->type == EVENT_PART) {
        feed_part(event);
    } else if (event->type == EVENT_QUIT) {
        feed_quit(event);
    } else if (event->type == EVENT_NICK) {
        feed_nick(event);
    } else {
        feed_channel(event);
    } 

    printf("\x1b[0m\r\n");
}
