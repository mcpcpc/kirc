/* See LICENSE file for license details. */
#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <termios.h>
#include <sys/ioctl.h>

#define VERSION     "0.2.5"              /* version */
#define MSG_MAX      512                 /* max message length */
#define CHA_MAX      200                 /* max channel length */
#define NIC_MAX      26                  /* max nickname length */
#define CTCP_CMDS   "ACTION VERSION TIME CLIENTINFO PING"
#define HBUFFER      100

static char   cdef[MSG_MAX] = "?";       /* default PRIVMSG channel */
static int    conn;                      /* connection socket */
static int    verb = 0;                  /* verbose output */
static int    sasl = 0;                  /* SASL method */
static char * host = "irc.freenode.net"; /* host address */
static char * port = "6667";             /* port */
static char * chan = NULL;               /* channel(s) */
static char * nick = NULL;               /* nickname */
static char * pass = NULL;               /* password */
static char * user = NULL;               /* user name */
static char * auth = NULL;               /* PLAIN SASL token */
static char * real = NULL;               /* real name */
static char * olog = NULL;               /* chat log path*/
static char * inic = NULL;               /* additional server command */

struct Param {
    char * prefix;
    char * suffix;
    char * message;
    char * nickname;
    char * command;
    char * channel;
    char * params;
    size_t offset;
    size_t maxcols;
    int    nicklen;
};

static struct termios orig;              /* restore at exit. */
static int    rawmode = 0;               /* check if restore is needed */
static int    atexit_registered = 0;     /* register atexit() */
static int    historyMaxLength = HBUFFER;/*  */
static int    history_len = 0;
static char **history = NULL;

struct State {
    char * prompt;                       /* Prompt to display. */
    char * buf;                          /* Edited line buffer. */
    size_t buflen;                       /* Edited line buffer size. */
    size_t plen;                         /* Prompt length. */
    size_t pos;                          /* Current cursor position. */
    size_t oldpos;                       /* Previous refresh cursor position. */
    size_t len;                          /* Current edited line length. */
    size_t cols;                         /* Number of columns in terminal. */
    size_t history_index;                 /* Current line in the edit history */
};

struct abuf {
    char * b;
    int    len;
};

static void freeHistory(void) {
    if (history) {
        int j;
        for (j = 0; j < history_len; j++) {
            free(history[j]);
        }
        free(history);
    }
}

static void disableRawMode(void) {
    if (rawmode && tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig) != -1) {
        rawmode = 0;
    }
    freeHistory();
}

static int enableRawMode(int fd) {
    if (!isatty(STDIN_FILENO))
        goto fatal;
    if (!atexit_registered) {
        atexit(disableRawMode);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig) == -1)
        goto fatal;
    struct termios raw = orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0)
        goto fatal;
    rawmode = 1;
    return 0;
fatal:
    errno = ENOTTY;
    return -1;
}

static int getCursorPosition(int ifd, int ofd) {
    char         buf[32];
    int          cols, rows;
    unsigned int i = 0;
    if (write(ofd, "\x1b[6n", 4) != 4)
        return -1;
    while (i < sizeof(buf) - 1) {
        if (read(ifd, buf + i, 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != 27 || buf[1] != '[')
        return -1;
    if (sscanf(buf+2, "%d;%d", &rows, &cols) != 2)
        return -1;
    return cols;
}

static int getColumns(int ifd, int ofd) {
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        int start = getCursorPosition(ifd, ofd);
        if (start == -1)
            return 80;
        if (write(ofd,"\x1b[999C",6) != 6)
            return 80;
        int cols = getCursorPosition(ifd, ofd);
        if (cols == -1)
            return 80;
        if (cols > start) {
            char seq[32];
            snprintf(seq, sizeof(seq), "\x1b[%dD", cols - start);
            if (write(ofd, seq, strnlen(seq, MSG_MAX)) == -1) {}
        }
        return cols;
    } else {
        return ws.ws_col;
    }
}

static void abInit(struct abuf * ab) {
    ab->b = NULL;
    ab->len = 0;
}

static void abAppend(struct abuf * ab, const char * s, int len) {
    char * new = realloc(ab->b, ab->len + len);
    if (new == NULL)
        return;
    memcpy(new + ab->len, s, len);
    ab->b = new;
    ab->len += len;
}

static void abFree(struct abuf * ab) {
    free(ab->b);
}

static void refreshLine(struct State * l) {
    char   seq[64];
    size_t plen = strlen(l->prompt) + 2;
    int    fd = STDOUT_FILENO;
    char * buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;
    struct abuf ab;
    l->cols = getColumns(STDIN_FILENO, STDOUT_FILENO);
    while (plen + pos >= l->cols) {
        buf++;
        len--;
        pos--;
    }
    while (plen + len > l->cols) {
        len--;
    }

    abInit(&ab);
    snprintf(seq, sizeof(seq), "\r");
    abAppend(&ab, seq, strnlen(seq, MSG_MAX));
    abAppend(&ab,l->prompt, strnlen(l->prompt, MSG_MAX));
    abAppend(&ab, "> ", 2);
    abAppend(&ab, buf, len);
    snprintf(seq, sizeof(seq), "\x1b[0K");
    abAppend(&ab, seq, strnlen(seq, MSG_MAX));
    snprintf(seq, sizeof(seq), "\r\x1b[%dC", (int)(pos + plen));
    abAppend(&ab, seq, strlen(seq));
    if (write(fd, ab.b, ab.len) == -1) {}
    abFree(&ab);
}

static int editInsert(struct State * l, char c) {
    if (l->len < l->buflen) {
        if (l->len == l->pos) {
            l->buf[l->pos] = c;
            l->pos++;
            l->len++;
            l->buf[l->len] = '\0';
            if (l->plen + l->len < l->cols) {
                char d = c;
                if (write(STDOUT_FILENO, &d, 1) == -1)
                    return -1;
            } else {
                refreshLine(l);
            }
        } else {
            memmove(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
            l->buf[l->pos] = c;
            l->len++;
            l->pos++;
            l->buf[l->len] = '\0';
            refreshLine(l);
        }
    }
    return 0;
}

static void editMoveLeft(struct State * l) {
    if (l->pos > 0) {
        l->pos--;
        refreshLine(l);
    }
}

static void editMoveRight(struct State * l) {
    if (l->pos != l->len) {
        l->pos++;
        refreshLine(l);
    }
}

static void editMoveHome(struct State * l) {
    if (l->pos != 0) {
        l->pos = 0;
        refreshLine(l);
    }
}

static void editMoveEnd(struct State * l) {
    if (l->pos != l->len) {
        l->pos = l->len;
        refreshLine(l);
    }
}

static void editDelete(struct State * l) {
    if (l->len > 0 && l->pos < l->len) {
        memmove(l->buf + l->pos, l->buf + l->pos + 1, l->len - l->pos - 1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void editBackspace(struct State * l) {
    if (l->pos > 0 && l->len > 0) {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void editDeletePrevWord(struct State * l) {
    size_t old_pos = l->pos;
    while (l->pos > 0 && l->buf[l->pos - 1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos - 1] != ' ')
        l->pos--;
    size_t diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

static void editDeleteWholeLine(struct State * l) {
    l->buf[0] = '\0';
    l->pos = l->len = 0;
    refreshLine(l);
}

static void editDeleteLineToEnd(struct State * l) {
    l->buf[l->pos] = '\0';
    l->len = l->pos;
    refreshLine(l);
}

static void editSwapCharWithPrev(struct State * l) {
    if (l->pos > 0 && l->pos < l->len) {
        int aux = l->buf[l->pos - 1];
        l->buf[l->pos - 1] = l->buf[l->pos];
        l->buf[l->pos] = aux;
        if (l->pos != l->len - 1)
            l->pos++;
        refreshLine(l);
    }
}

static void editHistory(struct Stat * l, int dir) {
    if (history_len > 1) {
        free(history[history_len - (1 + l->history_index)];
        history[history_len - (l->history_index)] = strdup(l->buf);
        l->history_index += (dir == 1) ? 1 : -1; /* 1 = previous */
        if (l->history_index < 0) {
            l->history_index = 0;
            return;
        } else if (l->history_index >= history_len) {
            l->history_index = history_len - 1;
            return;
        }
        strncpy(l->buf, history[history_len - (1 + l->history_index)], l->buflen);
        l->buf[l->buflen - 1] = '\0';
        l->len = l->pos = strnlen(l->buf, MSG_MAX);
        refreshLine(l);
    }
}

static int historyAdd(const char *line) {
    char *linecopy;

    if (history_max_len == 0) return 0;
    if (history == NULL) {
        history = malloc(sizeof(char*)*history_max_len);
        if (history == NULL) return 0;
        memset(history,0,(sizeof(char*)*history_max_len));
    }
    if (history_len && !strcmp(history[history_len-1], line)) return 0;
    linecopy = strdup(line);
    if (!linecopy) return 0;
    if (history_len == history_max_len) {
        free(history[0]);
        memmove(history,history+1,sizeof(char*)*(history_max_len-1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

static int edit(struct State * l) {
    char    c, seq[3];
    ssize_t nread = read(STDIN_FILENO, &c, 1);
    if (nread <= 0)
        return 1;
    switch(c) {
    case 13:                      return 1;  /* enter */
    case 3: errno = EAGAIN;       return -1; /* ctrl-c */
    case 127:                                /* backspace */
    case 8:  editBackspace(l);        break; /* ctrl-h */
    case 2:  editMoveLeft(l);         break; /* ctrl-b */
    case 6:  editMoveRight(l);        break; /* ctrl-f */
    case 1:  editMoveHome(l);         break; /* Ctrl+a */
    case 5:  editMoveEnd(l);          break; /* ctrl+e */
    case 23: editDeletePrevWord(l);   break; /* ctrl+w */
    case 21: editDeleteWholeLine(l);  break; /* Ctrl+u */
    case 11: editDeleteLineToEnd(l);  break; /* Ctrl+k */
    case 14: editHistory(l, 0);       break; /* Ctrl+n */
    case 16: editHistory(l, 1);       break; /* Ctrl+p */
    case 20: editSwapCharWithPrev(l); break; /* ctrl-t */
    case 4:                                  /* ctrl-d */
        if (l->len > 0) {
            editDelete(l);
        } else {
            return -1;
        }
        break;
    case 27:    /* escape sequence */
        if (read(STDIN_FILENO, seq, 1) == -1) break;
        if (read(STDIN_FILENO, seq + 1, 1) == -1) break;
        if (seq[0] == '[') { /* ESC [ sequences. */
            if (seq[1] >= '0' && seq[1] <= '9') {
                /* Extended escape, read additional byte. */
                if (read(STDIN_FILENO, seq + 2, 1) == -1) break;
                if (seq[2] == '~') {
                    if (seq[1] == 3) editDelete(l);    /* Delete key. */
                }
            } else {
                switch(seq[1]) {
                case 'A': editHistory(l, 1); break; /* Up */
                case 'b': editHistory(l, 0); break; /* Down */
                case 'C': editMoveRight(l);  break; /* Right */
                case 'D': editMoveLeft(l);   break; /* Left */
                case 'H': editMoveHome(l);   break; /* Home */
                case 'F': editMoveEnd(l);    break; /* End*/
                }
            }
        }
        else if (seq[0] == 'O') { /* ESC O sequences. */
            switch(seq[1]) {
            case 'H': editMoveHome(l); break; /* Home */
            case 'F': editMoveEnd(l);  break; /* End*/
            }
        }
        break;
    default: if (editInsert(l, c)) return -1; break;
    }
    return 0;
}

static void stateReset(struct State * l) {
    l->plen = strlen(l->prompt);
    l->oldpos = 0;
    l->pos = 0;
    l->len = 0;
    l->buf[0] = '\0';
    l->buflen--; 
}

static char * ctime_now(char buf[26]) {
    struct tm tm_;
    time_t now = time(NULL);
    if (!asctime_r(localtime_r (&now, &tm_), buf))
        return NULL;
    *strchr(buf, '\n') = '\0';
    return buf;
}

static void logAppend(char * str, char * path) {
    FILE * out;
    char buf[26];
    if ((out = fopen(path, "a")) == NULL) {
        perror("fopen");
        exit(1);
    }
    ctime_now(buf);
    fprintf(out, "%s:%s", buf, str);
    fclose(out);
}

static void raw(char * fmt, ...) {
    va_list ap;
    char *cmd_str = malloc(MSG_MAX);
    if (!cmd_str) {
        perror("malloc");
        exit(1);
    }
    va_start(ap, fmt);
    vsnprintf(cmd_str, MSG_MAX, fmt, ap);
    va_end(ap);
    if (verb)
        printf("<< %s", cmd_str);
    if (olog)
        logAppend(cmd_str, olog);
    if (write(conn, cmd_str, strnlen(cmd_str, MSG_MAX)) < 0) {
        perror("write");
        exit(1);
    }
    free(cmd_str);
}

static int initConnection(void) {
    int    gai_status;
    struct addrinfo *res, hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM
    };
    if ((gai_status = getaddrinfo(host, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai_status));
        return -1;
    }
    struct addrinfo *p;
    for (p = res; p != NULL; p = p->ai_next) {
        if ((conn = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket");
            continue;
        }
        if (connect(conn, p->ai_addr, p->ai_addrlen) == -1) {
            close(conn);
            perror("connect");
            continue;
        }
        break;
    }
    freeaddrinfo(res);
    if (p == NULL) {
        fputs("Failed to connect\n", stderr);
        return -1;
    }
    int flags = fcntl(conn, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(conn, F_SETFL, flags);

    return 0;
}

static void messageWrap(struct Param * p) {
    if (!p->message)
        return;
    char * tok;
    size_t wordwidth, spacewidth = 1;
    size_t spaceleft = p->maxcols - p->nicklen - p->offset;
    for (tok = strtok(p->message, " "); tok != NULL; tok = strtok(NULL, " ")) {
        wordwidth = strnlen(tok, MSG_MAX);
        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\r\n%*.s%s ", (int) p->nicklen + 1, " ", tok);
            spaceleft = p->maxcols - (p->nicklen + 1);
        } else {
            printf("%s ", tok);
        }
        spaceleft -= (wordwidth + spacewidth);
    }
}

static void paramPrintNick(struct Param * p) {
    printf("\x1b[35;1m%*s\x1b[0m ", p->nicklen - 4, p->nickname);
    printf("--> \x1b[35;1m%s\x1b[0m", p->message);
}

static void paramPrintPart(struct Param * p) {
    printf("%*s<-- \x1b[34;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
    if (p->channel != NULL && strcmp(p->channel+1, cdef))
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
}

static void paramPrintQuit(struct Param * p) {
    printf("%*s<<< \x1b[34;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
}

static void paramPrintJoin(struct Param * p) {
    printf("%*s--> \x1b[32;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
    if (p->channel != NULL && strcmp(p->channel+1, cdef))
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
}

static void handleCTCP(const char * nickname, char * message) {
    if (message[0] != '\001' && strncmp(message, "ACTION", 6))
        return;
    message++;
    if (!strncmp(message, "VERSION", 7)) {
        raw("NOTICE %s :\001VERSION kirc " VERSION "\001\r\n", nickname);
    } else if (!strncmp(message, "TIME", 7)) {
        char buf[26];
        if (!ctime_now(buf))
            raw("NOTICE %s :\001TIME %s\001\r\n", nickname, buf);
    } else if (!strncmp(message, "CLIENTINFO", 10)) {
        raw("NOTICE %s :\001CLIENTINFO " CTCP_CMDS "\001\r\n", nickname);
    } else if (!strncmp(message, "PING", 4)) {
        raw("NOTICE %s :\001%s\r\n", nickname, message);
    }
}

static void paramPrintPriv(struct Param * p) {
    int s = 0;
    if (strnlen(p->nickname, MSG_MAX) <= p->nicklen)
        s = p->nicklen - strnlen(p->nickname, MSG_MAX);
    if (p->channel != NULL && strcmp(p->channel, nick) == 0) {
        handleCTCP(p->nickname, p->message);
        printf("%*s\x1b[33;1m%-.*s\x1b[36m ", s, "", p->nicklen, p->nickname);
    } else if (p->channel != NULL && strcmp(p->channel + 1, cdef)) {
        printf("%*s\x1b[33;1m%-.*s\x1b[0m", s, "", p->nicklen, p->nickname);
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
        p->offset += 12 + strnlen(p->channel, CHA_MAX);
    } else {
        printf("%*s\x1b[33;1m%-.*s\x1b[0m ", s, "", p->nicklen, p->nickname);
    }
    if (!strncmp(p->message, "\x01""ACTION", 7)) {
        p->message += 7;
        p->offset += 10;
        printf("[ACTION] ");
    }
}

static void paramPrintChan(struct Param * p) {
    int s = 0;
    if (strnlen(p->nickname, MSG_MAX) <= p->nicklen)
        s = p->nicklen - strnlen(p->nickname, MSG_MAX);
    printf("%*s\x1b[33;1m%-.*s\x1b[0m ", s, "", p->nicklen, p->nickname);
    if (p->params) {
       printf("%s", p->params);
       p->offset += strnlen(p->params, CHA_MAX);
    }
}

static void rawParser(char * string) {
    if (!strncmp(string, "PING", 4)) {
        string[1] = 'O';
        raw("%s\r\n", string);
        return;
    }
    if (string[0] != ':' || (strnlen(string, MSG_MAX) < 4))
        return;
    printf("\r\x1b[0K");
    if (verb)
        printf(">> %s", string);
    if (olog)
        logAppend(string, olog);
    char * tok;
    struct Param p;
    p.prefix =   strtok(string, " ") + 1;
    p.suffix =   strtok(NULL, ":");
    p.message =  strtok(NULL, "\r");
    p.nickname = strtok(p.prefix, "!");
    p.command =  strtok(p.suffix, "#& ");
    p.channel =  strtok(NULL, " \r");
    p.params =   strtok(NULL, ":\r");
    p.maxcols = getColumns(STDIN_FILENO, STDOUT_FILENO);
    p.nicklen = (p.maxcols / 3 > NIC_MAX ? NIC_MAX : p.maxcols / 3);
    p.offset = 0;
    if (!strncmp(p.command, "001", 3) && chan != NULL) {
        for (tok = strtok(chan, ",|"); tok != NULL; tok = strtok(NULL, ",|")) {
            strcpy(cdef, tok);
            raw("JOIN #%s\r\n", tok);
        } return;
    } else if (!strncmp(p.command, "QUIT", 4)) {
        paramPrintQuit(&p);
    } else if (!strncmp(p.command, "PART", 4)) {
        paramPrintPart(&p);
    } else if (!strncmp(p.command, "JOIN", 4)) {
        paramPrintJoin(&p);
    } else if (!strncmp(p.command, "NICK", 4)) {
        paramPrintNick(&p);
    } else if (!strncmp(p.command, "PRIVMSG", 7)) {
        paramPrintPriv(&p);
        messageWrap(&p);
    } else {
        paramPrintChan(&p);
        messageWrap(&p);
    }
    printf("\x1b[0m\r\n");
}

static char   message_buffer[MSG_MAX + 1];
static size_t message_end = 0;

static int handleServerMessage(void) {
    for (;;) {
        ssize_t nread = read(conn, &message_buffer[message_end], MSG_MAX - message_end);
        if (nread == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            } else {
                perror("read");
                return -2;
            }
        }
        if (nread == 0) {
            fputs("\rconnection closed", stderr);
            puts("\r\x1b[E");
            return -1;
        }
        size_t i, old_message_end = message_end;
        message_end += nread;
        for (i = old_message_end; i < message_end; ++i) {
            if (i != 0 && message_buffer[i - 1] == '\r' && message_buffer[i] == '\n') {
                char saved_char = message_buffer[i + 1];
                message_buffer[i + 1] = '\0';
                rawParser(message_buffer);
                message_buffer[i + 1] = saved_char;
                memmove(&message_buffer, &message_buffer[i + 1], message_end - i - 1);
                message_end = message_end - i - 1;
                i = 0;
            }
        }
        if (message_end == MSG_MAX)
            message_end = 0;
    }
}

static void handleUserInput(struct State * l) {
    if (l->buf == NULL) {
        return;
    }
    historyAdd(l->buf);
    char * tok;
    size_t msg_len = strnlen(l->buf, MSG_MAX);
    if (msg_len > 0 && l->buf[msg_len - 1] == '\n')
        l->buf[msg_len - 1] = '\0';
    printf("\r\x1b[0K");
    switch (l->buf[0]) {
    case '/' : /* send system command */
        if (l->buf[1] == '#') {
            strcpy(cdef, l->buf + 2);
            printf("\x1b[35mnew channel: #%s\x1b[0m\r\n", cdef);
        } else {
            raw("%s\r\n", l->buf + 1);
            printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
        }
        break;
    case '@' : /* send private message to target channel or user */
        strtok_r(l->buf, " ", &tok);
        if (l->buf[1] == '@') {
            raw("privmsg %s :\001ACTION %s\001\r\n", l->buf + 2, tok);
            printf("\x1b[35mprivmsg %s :ACTION %s\x1b[0m\r\n", l->buf + 2, tok);
        } else {
            raw("privmsg %s :%s\r\n", l->buf + 1, tok);
            printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", l->buf + 1, tok);
        } break;
    default  : /*  send private message to default channel */
        raw("privmsg #%s :%s\r\n", cdef, l->buf);
        printf("\x1b[35mprivmsg #%s :%s\x1b[0m\r\n", cdef, l->buf);
    }
}

static void usage(void) {
    fputs("kirc [-s host] [-p port] [-c channel] [-n nick] [-r realname] \
[-u username] [-k password] [-a token] [-x command] [-o path] [-e] [-v] [-V]\n", stderr);
    exit(2);
}

static void version(void) {
    fputs("kirc-" VERSION "Copyright © 2021 Michael Czigler, MIT License\n", stdout);
    exit(0);
}

int main(int argc, char **argv) {
    int cval;
    while ((cval = getopt(argc, argv, "s:p:o:n:k:c:u:r:x:a:evV")) != -1) {
        switch (cval) {
        case 'v' : version();     break;
        case 'V' : ++verb;        break;
        case 'e' : ++sasl;        break;
        case 's' : host = optarg; break;
        case 'p' : port = optarg; break;
        case 'r' : real = optarg; break;
        case 'u' : user = optarg; break;
        case 'a' : auth = optarg; break;
        case 'o' : olog = optarg; break;
        case 'n' : nick = optarg; break;
        case 'k' : pass = optarg; break;
        case 'c' : chan = optarg; break;
        case 'x' : inic = optarg; break;
        case '?' : usage();       break;
        }
    }
    if (!nick) {
        fputs("Nick not specified\n", stderr);
        usage();
    }
    if (initConnection() != 0)
        return 1;
    if (auth || sasl)
        raw("CAP REQ :sasl\r\n");
    raw("NICK %s\r\n", nick);
    raw("USER %s - - :%s\r\n", (user ? user : nick), (real ? real : nick));
    if (auth || sasl) {
        raw("AUTHENTICATE %s\r\n", (sasl ? "EXTERNAL" : "PLAIN"));
        raw("AUTHENTICATE %s\r\nCAP END\r\n", (sasl ? "+" : auth));
    }
    if (pass)
        raw("PASS %s\r\n", pass);
    if (inic)
        raw("%s\r\n", inic);
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[1].fd = conn;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;
    char usrin[MSG_MAX];
    struct State l;
    l.buf = usrin;
    l.buflen = MSG_MAX;
    l.prompt = cdef;
    stateReset(&l);
    int rc, editReturnFlag = 0;
    if (enableRawMode(STDIN_FILENO) == -1)
        return 1;
    for (;;) {
        if (poll(fds, 2, -1) != -1) {
            if (fds[0].revents & POLLIN) {
                editReturnFlag = edit(&l);
                if (editReturnFlag > 0) {
                    handleUserInput(&l);
                    stateReset(&l);
                } else if (editReturnFlag < 0) {
                   printf("\r\n");
                   return 0;
                }
                refreshLine(&l);
            }
            if (fds[1].revents & POLLIN) {
                rc = handleServerMessage();
                if (rc != 0) {
                    if (rc == -2)
                        return 1;
                    return 0;
                }
                refreshLine(&l);
            }
        } else {
            if (errno == EAGAIN)
                continue;
            perror("poll");
            return 1;
        }
    }
}
