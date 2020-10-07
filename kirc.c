/* See LICENSE file for license details. */
#define _POSIX_C_SOURCE 200809L
#include <stdarg.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#define VERSION "0.1.4"

#define MSG_MAX              512                 /* max message length */
#define CHA_MAX              200                 /* max channel length */

static int            conn;                      /* connection socket */
static char           chan_default[MSG_MAX];     /* default PRIVMSG channel */
static int            verb = 0;                  /* verbose output */
static int            sasl = 0;                  /* SASL method */
static size_t         gutl = 20;                 /* max printed nick chars */
static char         * host = "irc.freenode.org"; /* host address */
static char         * port = "6667";             /* port */
static char         * chan = NULL;               /* channel(s) */
static char         * nick = NULL;               /* nickname */
static char         * pass = NULL;               /* password */
static char         * user = NULL;               /* user name */
static char         * auth = NULL;               /* PLAIN SASL token */
static char         * real = NULL;               /* real name */
static char         * olog = NULL;               /* chat log path*/
static char         * inic = NULL;               /* additional server command */

static struct         termios orig;
static int            rawmode = 0;
static int            atexit_registered = 0;

struct State {
    int               ifd;    /* Terminal stdin file descriptor. */
    int               ofd;    /* Terminal stdout file descriptor. */
    char             *buf;    /* Edited line buffer. */
    size_t            buflen; /* Edited line buffer size. */
    size_t            pos;    /* Current cursor position. */
    size_t            oldpos; /* Previous refresh cursor position. */
    size_t            len;    /* Current edited line length. */
    size_t            cols;   /* Number of columns in terminal. */
};

static void disableRawMode() {
    if (rawmode && tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig) != -1)
        rawmode = 0;
}

static int enableRawMode(int fd) {
    struct termios raw;

    if (!isatty(STDIN_FILENO)) {
        errno = ENOTTY;
        return -1;
    }
    if (!atexit_registered) {
        atexit(disableRawMode);
        atexit_registered = 1;
    }
    if (tcgetattr(fd,&orig) == -1) {
        errno = ENOTTY;
        return -1;
    } 

    raw = orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1; 
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) {
        errno = ENOTTY;
        return -1;
    }

    rawmode = 1;
    return 0;
}

static int getCursorPosition(int ifd, int ofd) {
    char buf[32];
    int cols, rows;
    unsigned int i = 0;

    if (write(ofd, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != 27 || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",&rows,&cols) != 2) return -1;
    return cols;
}

static int getColumns(int ifd, int ofd) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        int start, cols;

        start = getCursorPosition(ifd,ofd);
        if (start == -1) return 80;

        if (write(ofd,"\x1b[999C",6) != 6) return 80;
        cols = getCursorPosition(ifd,ofd);
        if (cols == -1) return 80;

        if (cols > start) {
            char seq[32];
            snprintf(seq,32,"\x1b[%dD",cols-start);
            if (write(ofd,seq,strlen(seq)) == -1) {}
        }
        return cols;
    } else {
        return ws.ws_col;
    }
}

struct abuf {
    char *b;
    int len;
};

static void abInit(struct abuf *ab) {
    ab->b = NULL;
    ab->len = 0;
}

static void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b,ab->len+len);

    if (new == NULL) return;
    memcpy(new+ab->len,s,len);
    ab->b = new;
    ab->len += len;
}

static void abFree(struct abuf *ab) {
    free(ab->b);
}

static void refreshLine(struct State *l) {
    char seq[64];
    int fd = l->ofd;
    char *buf = l->buf;
    size_t len = l->len;
    size_t pos = l->pos;
    struct abuf ab;

    while(pos >= l->cols) {
        buf++;
        len--;
        pos--;
    }
    while (len > l->cols) {
        len--;
    }

    abInit(&ab);
    snprintf(seq,64,"\r");
    abAppend(&ab,seq,strlen(seq));
    abAppend(&ab,buf,len);
    snprintf(seq,64,"\x1b[0K");
    abAppend(&ab,seq,strlen(seq));
    snprintf(seq,64,"\r\x1b[%dC", (int)(pos));
    abAppend(&ab,seq,strlen(seq));
    if (write(fd,ab.b,ab.len) == -1) {} /* Can't recover from write error. */
    abFree(&ab);
}

static int editInsert(struct State *l, char c) {
    if (l->len < l->buflen) {
        if (l->len == l->pos) {
            l->buf[l->pos] = c;
            l->pos++;
            l->len++;
            l->buf[l->len] = '\0';
            if ((l->len < l->cols)) {
                char d = c;
                if (write(l->ofd,&d,1) == -1) return -1;
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

static void editMoveLeft(struct State *l) {
    if (l->pos > 0) {
        l->pos--;
        refreshLine(l);
    }
}

static void editMoveRight(struct State *l) {
    if (l->pos != l->len) {
        l->pos++;
        refreshLine(l);
    }
}

static void editMoveHome(struct State *l) {
    if (l->pos != 0) {
        l->pos = 0;
        refreshLine(l);
    }
}

static void editMoveEnd(struct State *l) {
    if (l->pos != l->len) {
        l->pos = l->len;
        refreshLine(l);
    }
}

static void editDelete(struct State *l) {
    if (l->len > 0 && l->pos < l->len) {
        memmove(l->buf+l->pos,l->buf+l->pos+1,l->len-l->pos-1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void editBackspace(struct State *l) {
    if (l->pos > 0 && l->len > 0) {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void editDeletePrevWord(struct State *l) {
    size_t old_pos = l->pos;
    size_t diff;

    while (l->pos > 0 && l->buf[l->pos-1] == ' ')
        l->pos--;
    while (l->pos > 0 && l->buf[l->pos-1] != ' ')
        l->pos--;
    diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

static int edit(char *buf, size_t buflen)
{
    struct State l;

    l.ifd = STDIN_FILENO;
    l.ofd = STDOUT_FILENO;
    l.buf = buf;
    l.buflen = buflen;
    l.oldpos = l.pos = 0;
    l.len = 0;
    l.cols = getColumns(STDIN_FILENO, STDOUT_FILENO);

    l.buf[0] = '\0';
    l.buflen--; /* Make sure there is always space for the nulterm */

    while(1) {
        char c;
        int nread;
        char seq[3];

        nread = read(l.ifd,&c,1);
        if (nread <= 0) return l.len;

        switch(c) {
            case 3:     /* ctrl-c */
                errno = EAGAIN;
                return -1;
            case 4:
                if (l.len > 0) {
                    editDelete(&l);
                } else {
                    return -1;
                }
                break;
            case 13:  return (int)l.len;              /* enter */
            case 127:                                 /* backspace */
            case 8:   editBackspace(&l);       break; /* backspace */
            case 2:   editMoveLeft(&l);        break; /* ctrl+b */
            case 6:   editMoveRight(&l);       break; /* ctrl+f */
            case 1:   editMoveHome(&l);        break; /* ctrl+a */
            case 5:   editMoveEnd(&l);         break; /* ctrl+e */
            case 23:  editDeletePrevWord(&l);  break; /* ctrl+w */
            case 27:                                  /* esc sequence */
                if (read(l.ifd,seq,1) == -1)   break;
                if (read(l.ifd,seq+1,1) == -1) break;
                if (seq[0] == '[') {
                    if (seq[1] >= '0' && seq[1] <= '9') {
                        if (read(l.ifd,seq+2,1) == -1) break;
                        if (seq[2] == '~') {
                            switch(seq[1]) {
                            case '3': editDelete(&l); break;
                            }
                        }
                    } else {
                        switch(seq[1]) {
                            case 'C': editMoveRight(&l); break;
                            case 'D': editMoveLeft(&l);  break;
                            case 'H': editMoveHome(&l);  break;
                            case 'F': editMoveEnd(&l);   break;
                        }
                    }
                } else if (seq[0] == 'O') {
                    switch(seq[1]) {
                        case 'H': editMoveHome(&l); break;
                        case 'F': editMoveEnd(&l);  break;
                    }
                }
                break;
            case 21: /* Ctrl+u, delete the whole line. */
                buf[0] = '\0';
                l.pos = l.len = 0;
                refreshLine(&l);
                break;
            case 11: /* Ctrl+k, delete from current to end of line. */
                buf[l.pos] = '\0';
                l.len = l.pos;
                refreshLine(&l);
                break;
            default:
                if (editInsert(&l,c)) return -1;
                break;
        }
    }
    return l.len;
}

static void logAppend(char *str, char *path) {
    FILE *out;

    if ((out = fopen(path, "a")) == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(out, "%s\n", str);
    fclose(out);
}

static void raw(char *fmt, ...) {
    va_list ap;
    char *cmd_str = malloc(MSG_MAX);

    if (!cmd_str) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    va_start(ap, fmt);
    vsnprintf(cmd_str, MSG_MAX, fmt, ap);
    va_end(ap);

    if (verb) printf("<< %s", cmd_str);
    if (olog) logAppend(cmd_str, olog);
    if (write(conn, cmd_str, strlen(cmd_str)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    free(cmd_str);
}

static int connectionInit(void) {
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

static void messageWrap(char *line, size_t offset) {
    struct winsize window_dims;

    if (ioctl(0, TIOCGWINSZ, &window_dims) < 0) {
        perror("ioctrl");
        exit(EXIT_FAILURE);
    }

    unsigned short cmax = window_dims.ws_col;
    char          *tok;
    size_t         wordwidth, spaceleft = cmax - gutl - offset, spacewidth = 1;

    for (tok = strtok(line, " "); tok != NULL; tok = strtok(NULL, " ")) {
        wordwidth = strlen(tok);
        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\n%*.s%s ", (int) gutl + 1, " ", tok);
            spaceleft = cmax - (gutl + 1);
        } else {
            printf("%s ", tok);
        }
        spaceleft -= (wordwidth + spacewidth);
    }

    putchar('\n');
}

static void rawParser(char *string) {
    if (verb) printf(">> %s", string);

    if (!strncmp(string, "PING", 4)) {
        string[1] = 'O';
        raw("%s\r\n", string);
        return;
    }

    if (string[0] != ':') return;

    if (olog) logAppend(string, olog);

    char *tok, *prefix = strtok(string, " ") + 1, *suffix = strtok(NULL, ":"),
         *message = strtok(NULL, "\r"), *nickname = strtok(prefix, "!"),
         *command = strtok(suffix, "#& "), *channel = strtok(NULL, " ");
    int  g = gutl, s = gutl - (strlen(nickname) <= gutl ? strlen(nickname) : gutl);
    size_t offset = 0;

    if (!strncmp(command, "001", 3) && chan != NULL) {
        for (tok = strtok(chan, ",|"); tok != NULL; tok = strtok(NULL, ",|")) {
            strcpy(chan_default, tok);
            raw("JOIN #%s\r\n", tok);
        } return;
    } else if (!strncmp(command, "QUIT", 4) || !strncmp(command, "PART", 4)) {
        printf("%*s<-- \x1b[34;1m%s\x1b[0m\n", g - 3, "", nickname);
        return;
    } else if (!strncmp(command, "JOIN", 4)) {
        printf("%*s--> \x1b[32;1m%s\x1b[0m\n", g - 3, "", nickname);
        return;
    } else if (!strncmp(command, "NICK", 4)) {
        printf("\x1b[35;1m%*s\x1b[0m --> \x1b[35;1m%s\x1b[0m\n", g - 4, nickname, message);
        return;
    } else if (!strncmp(command, "PRIVMSG", 7) && strcmp(channel, nick) == 0) {
        printf("%*s\x1b[43;1m%-.*s\x1b[0m ", s, "", g, nickname);
    } else if (!strncmp(command, "PRIVMSG", 7) && strstr(channel, chan_default) == NULL) {
        printf("%*s\x1b[33;1m%-.*s\x1b[0m [\x1b[33m%s\x1b[0m] ", s, "", \
        g, nickname, channel);
        offset += 12 + strlen(channel);
    } else printf("%*s\x1b[33;1m%-.*s\x1b[0m ", s, "", g, nickname);
    messageWrap((message ? message : " "), offset);
}

static char   message_buffer[MSG_MAX + 1];
static size_t message_end = 0;

static int handleServerMessage(void) {
    for (;;) {
        ssize_t sl = read(conn, &message_buffer[message_end], MSG_MAX - message_end);
        if (sl == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0;
            } else {
                perror("read");
                return -2;
            }
        }
        if (sl == 0) {
            fputs("Connection closed\n", stderr);
            return -1;
        }

        size_t i, old_message_end = message_end;
        message_end += sl;

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
        if (message_end == MSG_MAX) {
            message_end = 0;
        }
    }
}

static void handleUserInput(char *usrin) {
    char *tok;

    size_t msg_len = strlen(usrin);
    if (usrin[msg_len - 1] == '\n') {
        usrin[msg_len - 1] = '\0';
    }

    if (usrin[0] == '/' && usrin[1] == '#') {
        strcpy(chan_default, usrin + 2);
        printf("new channel: #%s\n", chan_default);
    } else if (usrin[0] == '/' && usrin[1] == '?' && msg_len == 3) {
        printf("current channel: #%s\n", chan_default);
    } else if (usrin[0] == '/') {
        raw("%s\r\n", usrin + 1);
    } else if (usrin[0] == '@') {
        strtok_r(usrin, " ", &tok);
        raw("privmsg %s :%s\r\n", usrin + 1, tok);
    } else {
        raw("privmsg #%s :%s\r\n", chan_default, usrin);
    }
}

static void usage(void) {
    fputs("kirc [-s host] [-p port] [-c channel] [-n nick] [-r realname] \
[-u username] [-k password] [-a token] [-x command] [-w columns] [-o path] \
[-e|v|V]\n", stderr);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    int cval;

    while ((cval = getopt(argc, argv, "s:p:o:n:k:c:u:r:x:w:a:evV")) != -1) {
        switch (cval) {
            case 'V' : ++verb;                break;
            case 'e' : ++sasl;                break;
            case 's' : host = optarg;         break;
            case 'p' : port = optarg;         break;
            case 'r' : real = optarg;         break;
            case 'u' : user = optarg;         break;
            case 'a' : auth = optarg;         break;
            case 'o' : olog = optarg;         break;
            case 'n' : nick = optarg;         break;
            case 'k' : pass = optarg;         break;
            case 'c' : chan = optarg;         break;
            case 'x' : inic = optarg;         break;
            case 'w' : gutl = atoi(optarg);   break;
            case 'v' : puts("kirc-" VERSION); break;
            case '?' : usage();               break;
        }
    }

    if (!nick) {
        fputs("Nick not specified\n", stderr);
        usage();
    }

    if (connectionInit() != 0) {
        return EXIT_FAILURE;
    }

    if (auth || sasl) raw("CAP REQ :sasl\r\n");
    raw("NICK %s\r\n", nick);
    raw("USER %s - - :%s\r\n", (user ? user : nick), (real ? real : nick));
    if (auth || sasl) {
        raw("AUTHENTICATE %s\r\n", (sasl ? "EXTERNAL" : "PLAIN"));
        raw("AUTHENTICATE %s\r\nCAP END\r\n", (sasl ? "+" : auth));
    }
    if (pass) raw("PASS %s\r\n", pass);
    if (inic) raw("%s\r\n", inic);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[1].fd = conn;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;

    char usrin[MSG_MAX];
    int  count, byteswaiting = 1;

    for (;;) {
        int poll_res = poll(fds, 2, -1);
        if (poll_res != -1) {
            if (fds[0].revents & POLLIN) {
                byteswaiting = 0;
                if (enableRawMode(STDIN_FILENO) == -1) return -1;
                count = edit(usrin, MSG_MAX);
                disableRawMode();
                handleUserInput(usrin);
                byteswaiting = 1;
            }
            if (fds[1].revents & POLLIN && byteswaiting) {
                int rc = handleServerMessage();
                if (rc != 0) {
                    if (rc == -2) return EXIT_FAILURE;
                    return EXIT_SUCCESS;
                };
            }
        } else {
            if (errno == EAGAIN) continue;
            perror("poll");
            return EXIT_FAILURE;
        }
    }
}
