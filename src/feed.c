#include "feed.h"

static size_t display_width(const char *s)
{
    size_t width = 0;
    mbstate_t ps = {0};

    while (*s) {
        wchar_t wc;
        size_t n = mbrtowc(&wc, s, MB_CUR_MAX, &ps);

        /* Invalid byte sequence → treat as single printable */
        if (n == (size_t)-1 || n == (size_t)-2) {
            width++;
            s++;
            memset(&ps, 0, sizeof(ps));
            continue;
        }

        /* Skip ANSI escape sequences */
        if (*s == '\x1b') {
            s++;
            if (*s == '[') {
                while (*s && !isalpha((unsigned char)*s))
                    s++;
                if (*s)
                    s++;
            }
            continue;
        }

        int w = wcwidth(wc);
        if (w > 0)
            width += w;

        s += n;
    }

    return width;
}

static void wordwrap(char *message, int cols, int lwidth)
{
    size_t wordwidth, spacewidth = 1;
    size_t spaceleft = cols - lwidth;

    for (char *tok = strtok(message, " "); tok != NULL;
         tok = strtok(NULL, " ")) {

        wordwidth = display_width(tok);

        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\r\n%*.s%s ", (int)lwidth + 1,
                " ", tok);
            spaceleft = cols - (lwidth + 1);
        } else {
            printf("%s ", tok);
        }

        spaceleft -= wordwidth + spacewidth;
    }
}

static void feed_privmsg(event_t *event)
{
    int cols = terminal_columns(event->ctx);
    int lwidth = event->ctx->lwidth;

    printf("\r\x1b[0K");

    if (strcmp(event->channel, event->ctx->nickname) == 0) {
        printf("\x1b[33;1m%-*s\x1b[0m ", lwidth,
            event->nickname);
    } else {
        printf("\x1b[1m%-*s\x1b[0m ", lwidth,
            event->nickname);
    }

    wordwrap(event->message, cols, lwidth);

    printf("\r\n");
}

static void feed_join(event_t *event)
{
    printf("\r\x1b[0K\x1b[2m--> %s\x1b[0m\r\n",
        event->nickname);
}

static void feed_part(event_t *event)
{
    printf("\r\x1b[0K\x1b[2m<-- %s\x1b[0m\r\n",
        event->nickname);
}

static void feed_quit(event_t *event)
{
    printf("\r\x1b[0K\x1b[2m<<< %s\x1b[0m\r\n",
        event->nickname);
}

static void feed_nick(event_t *event)
{
    printf("\r\x1b[0K\x1b[2m%s --> %s\x1b[0m\r\n",
        event->nickname, event->message);
}

static void feed_channel(event_t *event)
{
    int cols = terminal_columns(event->ctx);
    int lwidth = event->ctx->lwidth;

    printf("\r\x1b[0K");

    if (event->channel[0] != '\0') {
        printf("\x1b[1m%-*s\x1b[0m ", lwidth,
            event->channel);
    } else {
        printf("\x1b[1m%-*s\x1b[0m ", lwidth,
            event->nickname);
    }

    wordwrap(event->message, cols, lwidth);

    printf("\r\n");
}

void feed_render(event_t *event)
{
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

    default:
        feed_channel(event);
        break;
    }
}
