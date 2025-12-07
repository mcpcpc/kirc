#include "feed.h"

static int is_ansi_escape(const char *s)
{
    return s[0] == '\x1b' && s[1] == '[';
}

static size_t visible_width(const char *s)
{
    size_t width = 0;
    mbstate_t st = {0};
    wchar_t wc;

    while (*s) {
        if (is_ansi_escape(s)) {
            s += 2;
            while (*s && (*s < '@' || *s > '~'))
                s++;
            if (*s) s++;   // Final CSI byte
            continue;
        }

        size_t len = mbrtowc(&wc, s, MB_CUR_MAX, &st);
        if (len == (size_t)-1 || len == (size_t)-2) {
            ++s;
            ++width;
            continue;
        }

        int w = wcwidth(wc);
        width += (w > 0) ? w : 0;
        s += len;
    }

    return width;
}

static void wordwrap(char *message, int cols)
{
    size_t wordwidth, spacewidth = 1, nicklen = 17;
    size_t spaceleft = cols - (nicklen + 1);

    for (char *tok = strtok(message, " "); tok != NULL; tok = strtok(NULL, " ")) {

        wordwidth = visible_width(tok);

        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\r\n%*.s%s ", (int)nicklen + 1, " ", tok);
            spaceleft = cols - (nicklen + 1);
        } else {
            printf("%s ", tok);
        }

        spaceleft -= wordwidth + spacewidth;
    }
}

/*
static void wordwrap(char *message, int cols)
{
    size_t wordwidth, spacewidth = 1, nicklen = 17;
    size_t spaceleft = cols - (nicklen + 1);

    for (char *tok = strtok(message, " "); tok != NULL; tok = strtok(NULL, " ")) {
        wordwidth = strnlen(tok, RFC1459_MESSAGE_MAX_LEN);
        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\r\n%*.s%s ", (int)nicklen + 1, " ", tok);
            spaceleft = cols - (nicklen + 1);
        } else {
            printf("%s ", tok);
        }
        spaceleft -= wordwidth + spacewidth;
    }
}
*/

static void feed_privmsg(event_t *event)
{
    int cols = terminal_columns(event->ctx);

    char nickname[16] = {0};
    strncpy(nickname, event->nickname, 15);
    printf("%s: ", nickname);

    wordwrap(event->message, cols);
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
    int cols = terminal_columns(event->ctx);

    if (event->channel[0] != '\0') {
        printf("%s: ", event->channel);
    } else {
        printf("%s: ", event->nickname);
    }

    wordwrap(event->message, cols);
}

void feed_render(event_t *event)
{
    switch (event->type) {
    case EVENT_PING:
        break;

    case EVENT_PRIVMSG:
        feed_privmsg(event);
        printf("\x1b[0m\r\n");
        break;

    case EVENT_JOIN:
        feed_join(event);
        printf("\x1b[0m\r\n");
        break;

    case EVENT_PART:
        feed_part(event);
        printf("\x1b[0m\r\n");
        break;

    case EVENT_QUIT:
        feed_quit(event);
        printf("\x1b[0m\r\n");
        break;

    case EVENT_NICK:
        feed_nick(event);
        printf("\x1b[0m\r\n");
        break;

    default:
        feed_channel(event);
        printf("\x1b[0m\r\n");
        break;
    }
}
