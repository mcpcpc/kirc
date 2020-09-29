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

#define VERSION "0.1.3"

#define MSG_MAX      512                 /* guaranteed max message length */
#define CHA_MAX      200                 /* guaranteed max channel length */

static int    conn;                      /* connection socket */
static char   chan_default[MSG_MAX];     /* default channel for PRIVMSG */
static int    verb = 0;                  /* verbose output (e.g. raw stream) */
static int    sasl = 0;                  /* SASL method (PLAIN=0, EXTERNAL=1) */
static size_t cmax = 80;                 /* max number of chars per line */
static size_t gutl = 20;                 /* max char width of left column */
static char * host = "irc.freenode.org"; /* irc host address */
static char * port = "6667";             /* server port */
static char * chan = NULL;               /* channel(s) */
static char * nick = NULL;               /* nickname */
static char * pass = NULL;               /* server password */
static char * user = NULL;               /* server user name */
static char * auth = NULL;               /* PLAIN SASL authentication token */
static char * real = NULL;               /* server user real name */
static char * olog = NULL;               /* log irc stream path */
static char * inic = NULL;               /* server command after connection */

static void
log_append(char *str, char *path) {

    FILE *out;

    if ((out = fopen(path, "a")) == NULL) {
        perror("fopen");
        exit(1);
    }

    fprintf(out, "%s\n", str);
    fclose(out);
}

static void
raw(char *fmt, ...) {

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
    if (olog) log_append(cmd_str, olog);
    if (write(conn, cmd_str, strlen(cmd_str)) < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    free(cmd_str);
}

static int
connection_initialize(void) {

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

static void
message_wrap(char *line, size_t offset) {

    char    *tok;
    size_t  wordwidth, spaceleft = cmax - gutl - offset, spacewidth = 1;

    for(tok = strtok(line, " "); tok != NULL; tok = strtok(NULL, " ")) {
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

static void
raw_parser(char *string) {

    if (verb) printf(">> %s\n", string);

    if (!strncmp(string, "PING", 4)) {
        string[1] = 'O';
        raw("%s\r\n", string);
        return;
    }

    if (string[0] != ':') return;

    if (olog) log_append(string, olog);

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
    message_wrap((message ? message : " "), offset);
}

static char   message_buffer[MSG_MAX + 1];
static size_t message_end = 0;

static int
handle_server_message(void) {
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
                raw_parser(message_buffer);
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

static void
handle_user_input(void) {

    char usrin[MSG_MAX], *tok;

    if (fgets(usrin, MSG_MAX, stdin) == NULL) {
        perror("fgets");
        exit(1);
    }

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

static int
keyboard_hit() {

    struct termios save, tp;
    int    byteswaiting;

    tcgetattr(STDIN_FILENO, &tp);
    save = tp;
    tp.c_lflag &= ~ICANON;
    tcsetattr(STDIN_FILENO, TCSANOW, &tp);
    ioctl(STDIN_FILENO, FIONREAD, &byteswaiting);
    tcsetattr(STDIN_FILENO, TCSANOW, &save);

    return byteswaiting;
}

static void
usage(void) {
    fputs("kirc [-s hostname] [-p port] [-c channel] [-n nick] \
[-r real_name] [-u username] [-k password] [-a token] [-x init_command] \
[-w columns] [-W columns] [-o path] [-e|v|V]\n", stderr);
    exit(EXIT_FAILURE);
}

int
main(int argc, char **argv) {

    int cval;

    while ((cval = getopt(argc, argv, "s:p:o:n:k:c:u:r:x:w:W:a:hevV")) != -1) {
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
            case 'W' : cmax = atoi(optarg);   break;
            case 'v' : puts("kirc-" VERSION); break;
            case '?' : usage();               break;
        }
    }

    if (!nick) {
        fputs("Nick not specified\n", stderr);
        usage();
    }

    if (connection_initialize() != 0) {
        return EXIT_FAILURE;
    }

    if (auth || sasl) raw("CAP REQ :sasl\r\n");
    raw("NICK %s\r\n", nick);
    raw("USER %s - - :%s\r\n", (user ? user : nick), (real ? real : nick));
    if (auth || sasl) raw("AUTHENTICATE %s\r\n", (sasl ? "EXTERNAL" : "PLAIN"));
    if (auth || sasl) raw("AUTHENTICATE %s\r\n", (sasl ? "+" : auth));
    if (auth || sasl) raw("CAP END\r\n");
    if (pass) raw("PASS %s\r\n", pass);
    if (inic) raw("%s\r\n", inic);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[1].fd = conn;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;

    for (;;) {
        int poll_res = poll(fds, 2, -1);
        if (poll_res != -1) {
            if (fds[0].revents & POLLIN) {
                handle_user_input();
            }
            if (fds[1].revents & POLLIN && (keyboard_hit() < 1)) {
                int rc = handle_server_message();
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
