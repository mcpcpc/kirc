#include "parse.h"

static void reset_event(irc_event_t *ev)
{
    if (!ev) {
        return;
    }

    ev->type = IRC_EVENT_RAW;

    ev->origin.nick = NULL;
    ev->origin.user = NULL;
    ev->origin.host = NULL;

    ev->command = NULL;
    ev->channel = NULL;
    ev->message = NULL;
    ev->params  = NULL;

    ev->numeric = -1;
}

static int is_numeric_command(const char *cmd)
{
    if (!cmd) {
        return 0;
    }

    if (strlen(cmd) != 3) {
        return 0;
    }

    return isdigit((unsigned char)cmd[0]) &&
           isdigit((unsigned char)cmd[1]) &&
           isdigit((unsigned char)cmd[2]);
}

static void parse_prefix(char *prefix, irc_identity_t *id)
{
    char *bang;
    char *at;

    if (!prefix || !id) {
        return;
    }

    id->nick = NULL;
    id->user = NULL;
    id->host = NULL;

    bang = strchr(prefix, '!');
    at   = strchr(prefix, '@');

    if (bang) {
        *bang = '\0';
        id->nick = prefix;

        if (at && at > bang + 1) {
            *at = '\0';
            id->user = bang + 1;
            id->host = at + 1;
        } else if (*(bang + 1) != '\0') {
            id->user = bang + 1;
        }
    } else {
        /* No '!' => treat entire prefix as nick/host */
        id->nick = prefix;
    }
}

static void trim_crlf(char *s)
{
    size_t len;

    if (!s) {
        return;
    }

    len = strlen(s);
    while (len > 0 &&
           (s[len - 1] == '\r' || s[len - 1] == '\n')) {
        s[--len] = '\0';
    }
}

int parser_feed(parser_t *p, char *line, irc_event_t *out)
{
    (void)p;  /* parser_t reserved for future use */

    char *cur;
    char *prefix = NULL;
    char *cmd = NULL;
    char *params = NULL;
    char *trailing = NULL;

    if (!line || !out) {
        return -1;
    }

    reset_event(out);
    trim_crlf(line);

    if (line[0] == '\0') {
        return 0;   /* ignore empty */
    }

    if (strncmp(line, "PING", 4) == 0) {
        out->type    = IRC_EVENT_PING;
        out->command = "PING";

        cur = line + 4;
        while (*cur == ' ' || *cur == ':') {
            cur++;
        }
        out->message = (*cur != '\0') ? cur : NULL;

        return 1;
    }

    if (line[0] != ':') {
        return 0;   /* ignore non-prefixed lines */
    }

    cur = line + 1;

    char *space = strchr(cur, ' ');
    if (!space) {
        return 0;
    }
    *space = '\0';
    prefix = cur;

    cur = space + 1;
    while (*cur == ' ') {
        cur++;
    }

    cmd = cur;
    while (*cur && *cur != ' ') {
        cur++;
    }
    if (*cur) {
        *cur = '\0';
        cur++;
    }

    while (*cur == ' ') {
        cur++;
    }

    out->command = cmd;

    /* Parse numeric vs. named command */
    if (is_numeric_command(cmd)) {
        out->type    = IRC_EVENT_NUMERIC;
        out->numeric = atoi(cmd);
    } else if (strcmp(cmd, "PRIVMSG") == 0) {
        out->type = IRC_EVENT_PRIVMSG;
    } else if (strcmp(cmd, "JOIN") == 0) {
        out->type = IRC_EVENT_JOIN;
    } else if (strcmp(cmd, "PART") == 0) {
        out->type = IRC_EVENT_PART;
    } else if (strcmp(cmd, "QUIT") == 0) {
        out->type = IRC_EVENT_QUIT;
    } else if (strcmp(cmd, "NICK") == 0) {
        out->type = IRC_EVENT_NICK;
    } else {
        out->type = IRC_EVENT_RAW;
    }

    /* Parse prefix into identity fields */
    parse_prefix(prefix, &out->origin);

    /* Split parameters and trailing (message) by first " :" */
    if (*cur == ':') {
        trailing = cur + 1;
        params   = NULL;
    } else if (*cur != '\0') {
        char *colon = strstr(cur, " :");
        if (colon) {
            *colon = '\0';
            trailing = colon + 2;
            params   = (*cur != '\0') ? cur : NULL;
        } else {
            params   = (*cur != '\0') ? cur : NULL;
            trailing = NULL;
        }
    }

    out->params  = params;
    out->message = trailing;

    if (params) {
        char *first = params;
        while (*first == ' ') {
            first++;
        }
        if (*first) {
            char *end = strchr(first, ' ');
            if (end) {
                *end = '\0';
            }
            out->channel = first;
        }
    } else {
        out->channel = NULL;
    }

    return 1;
}
