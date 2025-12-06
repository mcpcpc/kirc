#include "render.h"

static int clamp_nick_width(int cols)
{
    int w = cols / 3;
    if (w < 8) {
        w = 8;
    }
    if (w > NIC_MAX) {
        w = NIC_MAX;
    }
    return w;
}

static const char *safe_str(const char *s)
{
    return s ? s : "";
}

/* Does ev->channel logically match the default channel? */
static int channel_is_default(const render_t *r,
        const char *channel)
{
    if (!channel || r->default_channel[0] == '\0') {
        return 0;
    }

    /* default_channel is assumed to be without leading '#' */
    const char *ch = channel;
    if (ch[0] == '#' || ch[0] == '&') {
        ch++;
    }

    return strcmp(ch, r->default_channel) == 0;
}

static void print_nick(const char *nick, int nick_width)
{
    const char *n = safe_str(nick);
    int len = (int)strlen(n);
    int pad = 0;

    if (len < nick_width) {
        pad = nick_width - len;
    }

    printf("%*s\x1b[33;1m%-.*s\x1b[0m", pad, "",
           nick_width, n);
}

static void render_privmsg(render_t *r, const irc_event_t *ev,
        int cols)
{
    (void) r; /* r is kept for future tweaks (e.g., theme, verbosity) */

    const char *nick   = safe_str(ev->origin.nick);
    const char *chan   = ev->channel;
    char       *msg    = ev->message; /* assumed mutable, as in original */
    int         width  = clamp_nick_width(cols);

    /* Left side: nickname + optional [#channel] */
    int s = 0;
    int nicklen = (int)strlen(nick);
    if (nicklen <= width) {
        s = width - nicklen;
    }

    printf("%*s", s, "");
    printf("\x1b[33;1m%-.*s\x1b[0m", width, nick);

    if (chan && !channel_is_default(r, chan)) {
        printf(" [\x1b[33m%s\x1b[0m] ", chan);
    } else {
        printf(" ");
    }

    /* Simple, single-line message print for now */
    printf("%s", safe_str(msg));
    printf("\x1b[0m\r\n");
}

static void render_join(render_t *r, const irc_event_t *ev,
        int cols)
{
    (void) r;
    int width = clamp_nick_width(cols);
    const char *nick = safe_str(ev->origin.nick);
    const char *chan = ev->channel;

    printf("%*s--> \x1b[32;1m%-.*s\x1b[0m",
           width - 3,
           "",
           width,
           nick);

    if (chan && !channel_is_default(r, chan)) {
        printf(" [\x1b[33m%s\x1b[0m]", chan);
    }

    printf("\x1b[0m\r\n");
}

static void render_part(render_t *r, const irc_event_t *ev,
        int cols)
{
    (void) r;
    int width = clamp_nick_width(cols);
    const char *nick = safe_str(ev->origin.nick);
    const char *chan = ev->channel;

    printf("%*s<-- \x1b[34;1m%-.*s\x1b[0m",
           width - 3,
           "",
           width,
           nick);

    if (chan && !channel_is_default(r, chan)) {
        printf(" [\x1b[33m%s\x1b[0m]", chan);
    }

    printf("\x1b[0m\r\n");
}

static void render_quit(render_t *r, const irc_event_t *ev,
        int cols)
{
    (void) r;
    int width = clamp_nick_width(cols);
    const char *nick = safe_str(ev->origin.nick);

    printf("%*s<<< \x1b[34;1m%-.*s\x1b[0m",
           width - 3,
           "",
           width,
           nick);

    if (ev->message) {
        printf(" (%s)", ev->message);
    }

    printf("\x1b[0m\r\n");
}

static void render_nick(render_t *r, const irc_event_t *ev,
        int cols)
{
    (void) r;
    int width = clamp_nick_width(cols);
    const char *nick = safe_str(ev->origin.nick);

    printf("%*s\x1b[35;1m%-.*s\x1b[0m ",
           width - 4,
           "",
           width,
           nick);

    if (ev->message) {
        printf("--> \x1b[35;1m%s\x1b[0m", ev->message);
    }

    printf("\x1b[0m\r\n");
}

static void render_numeric(render_t *r, const irc_event_t *ev,
        int cols)
{
    (void) cols;
    /* For numeric replies, just show raw-ish line in purple if verbose. */
    if (!r->verbose) {
        return;
    }

    printf("\x1b[35m[%03d] %s %s %s\x1b[0m\r\n",
        ev->numeric,
        safe_str(ev->command),
        safe_str(ev->params),
        safe_str(ev->message));
}

static void render_raw(render_t *r, const irc_event_t *ev)
{
    if (!r->verbose) {
        return;
    }
    printf("\x1b[36m%s\x1b[0m\r\n", safe_str(ev->message));
}

void render_event(render_t *r, const event_t *ev,
        int terminal_cols)
{
    if (!r || !ev) {
        return;
    }

    switch (ev->type) {
    case EVENT_PING:
        /* PING is usually handled at network/protocol level, not rendered. */
        break;

    case EVENT_PRIVMSG:
        render_privmsg(r, ev, terminal_cols);
        break;

    case EVENT_JOIN:
        render_join(r, ev, terminal_cols);
        break;

    case EVENT_PART:
        render_part(r, ev, terminal_cols);
        break;

    case EVENT_QUIT:
        render_quit(r, ev, terminal_cols);
        break;

    case EVENT_NICK:
        render_nick(r, ev, terminal_cols);
        break;

    case EVENT_NUMERIC:
        render_numeric(r, ev, terminal_cols);
        break;

    case EVENT_RAW:
    default:
        render_raw(r, ev);
        break;
    }
}

void render_editor_line(render_t *r, const editor_t *e)
{
    (void) r;

    if (!e || !e->buffer || !e->prompt) {
        return;
    }

    /* Move to start of line */
    printf("\r");

    /* Prompt + "> " */
    printf("%s> ", e->prompt);

    /* Buffer content */
    printf("%s", e->buffer);

    /* Clear to end of line */
    printf("\x1b[0K");

    /* Reposition cursor:
     * logical column = prompt_len_u8 + 2 ("> ") + cursor_u8
     */
    size_t col = e->prompt_len_u8 + 2 + e->cursor_u8;
    if (col > 0) {
        printf("\r\x1b[%zuC", col);
    } else {
        printf("\r");
    }

    fflush(stdout);
}

