/* See LICENSE file for license details. */
#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <stdarg.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <termios.h>

#define MSG_MAX       512                /* guaranteed max message length */
#define CHA_MAX       200                /* gauranteed max channel length */

static int    conn;                      /* connection socket */
static size_t verb = 0;                  /* verbose output (e.g. raw stream) */
static size_t cmax = 80;                 /* max number of chars per line */
static size_t gutl = 10;                 /* max char width of left column */
static char * host = "irc.freenode.org"; /* irc host address */
static char * chan = "kirc";             /* channel */
static char * port = "6667";             /* server port */
static char * nick = NULL;               /* nickname */
static char * pass = NULL;               /* server password */
static char * user = NULL;               /* server user name */
static char * real = NULL;               /* server user real name */
static char * olog = NULL;               /* log irc stream path */
static char * inic = NULL;               /* server command after connection */

static void
printa(char *str, char *path) {

    FILE *out = fopen(path, "a");

    fprintf(out, "%s", str);
    fclose(out);
}

static int
kbhit(void) {

    int    byteswaiting;
    struct timespec ts = {0};
    fd_set fds;

    FD_ZERO(&fds);
    FD_SET(0, &fds);
    byteswaiting = pselect(1, &fds, NULL, NULL, &ts, NULL);

    return byteswaiting > 0;
}

static void
raw(char *fmt, ...) {

    va_list ap;
    char *cmd_str = malloc(MSG_MAX);

    va_start(ap, fmt);
    vsnprintf(cmd_str, MSG_MAX, fmt, ap);
    va_end(ap);

    if (verb) printf("<< %s", cmd_str);
    if (olog) printa(cmd_str, olog);
    if (write(conn, cmd_str, strlen(cmd_str)) < 0) exit(1);

    free(cmd_str);
}

static void
irc_init() {

    struct addrinfo *res, hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };

    getaddrinfo(host, port, &hints, &res);
    conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(conn, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    fcntl(conn, F_SETFL, O_NONBLOCK);
}

static void
printw(const char *format, ...) {

    va_list argptr;
    char    *tok, line[MSG_MAX];
    size_t  i, wordwidth, spaceleft, spacewidth = 1;

    va_start(argptr, format);
    vsnprintf(line, MSG_MAX, format, argptr);
    va_end(argptr);

    if (olog) printa(line, olog);

    for (i = 0; isspace(line[i]); i++) putchar(line[i]);

    spaceleft = cmax + gutl - (i - 1);

    for(tok = strtok(&line[i], " "); tok != NULL; tok = strtok(NULL, " ")) {
        wordwidth = strlen(tok);

        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\n%*.s%s ", (int) gutl + 1, "", tok);
            spaceleft = cmax - (gutl + 1 + wordwidth);
        } else {
            printf("%s ", tok);
            spaceleft = spaceleft - (wordwidth + spacewidth);
        }
    }

    printf("\n");
}

static void
raw_parser(char *usrin) {

    if (verb) printf(">> %s\n", usrin);

    if (!strncmp(usrin, "PING", 4)) {
        usrin[1] = 'O';
        raw("%s\r\n", usrin);
    } else if (usrin[0] == ':') {

        char *prefix = strtok(usrin, " ") + 1, *suffix = strtok(NULL, ":"),
             *message = strtok(NULL, "\r"), *nickname = strtok(prefix, "!"),
             *command = strtok(suffix, "#& "), *channel = strtok(NULL, " ");

        if (!strncmp(command, "001", 3)) {
            raw("JOIN #%s\r\n", chan);
        } else if (!strncmp(command, "QUIT", 4)) {
            printw("%*s<-- \x1b[34;1m%s\x1b[0m", (int)gutl - 3, "", nickname);
        } else if (!strncmp(command, "JOIN", 4)) {
            printw("%*s--> \x1b[32;1m%s\x1b[0m", (int)gutl - 3, "", nickname);
        } else if (!strncmp(command, "PRIVMSG", 7) && 
                   !strncmp(channel, nick, strlen(nick))) {
            int s = gutl - (strlen(nickname) <= gutl ? strlen(nickname) : gutl);
            printw("%*s\x1b[43;1m%-.*s\x1b[0m %s", s, "", (int)gutl, nickname, message);
        } else if (!strncmp(command, "PRIVMSG", 7) && strstr(channel, chan) == NULL) {
            int s = gutl - (strlen(nickname) <= gutl ? strlen(nickname) : gutl);
            printw("%*s\x1b[33;1m%-.*s\x1b[0m [%s] %s", s, "", (int)gutl, nickname, channel, message);
        } else {
            int s = gutl - (strlen(nickname) <= gutl ? strlen(nickname) : gutl);
            printw("%*s\x1b[33;1m%-.*s\x1b[0m %s", s, "", (int)gutl, nickname, message);
        }
    }
}

int
main(int argc, char **argv) {

    int fd[2], cval;

    while ((cval = getopt(argc, argv, "s:p:o:n:k:c:u:r:x:w:W:vV")) != -1) {
        switch (cval) {
            case 'v' : puts("kirc-0.0.8");  return 0;
            case 'V' : verb = 1;            break;
            case 's' : host = optarg;       break;
            case 'w' : gutl = atoi(optarg); break;
            case 'W' : cmax = atoi(optarg); break;
            case 'p' : port = optarg;       break;
            case 'r' : real = optarg;       break;
            case 'u' : user = optarg;       break;
            case 'o' : olog = optarg;       break;
            case 'n' : nick = optarg;       break;
            case 'k' : pass = optarg;       break;
            case 'c' : chan = optarg;       break;
            case 'x' : inic = optarg;       break;
            case '?' :                      return 1;
        }
    }

    if (!nick) {
        fprintf(stderr, "nick not specified\n");
        return 1;
    }

    if (pipe(fd) < 0) {
        fprintf(stderr, "fork() failed\n");
        return 1;
    }

    pid_t pid = fork();

    if (pid == 0) {

        int  sl, i, o = 0;
        char u[MSG_MAX], s, b[MSG_MAX];

        irc_init();

        raw("NICK %s\r\n", nick);
        raw("USER %s - - :%s\r\n", (user ? user : nick), (real ? real : nick));

        if (pass) raw("PASS %s\r\n", pass);
        if (inic) raw("%s\r\n", inic);

        while ((sl = read(conn, &s, 1))) {
            if (sl > 0) b[o] = s;

            if ((o > 0 && b[o - 1] == '\r' && b[o] == '\n') || o == MSG_MAX) {
                b[o + 1] = '\0';
                raw_parser(b);
                o = 0;
            } else if (sl > 0) o++;

            if (read(fd[0], u, MSG_MAX) > 0) {
                for (i = 0; u[i] != '\n'; i++) continue;
                if (u[0] != '/') raw("%-*.*s\r\n", i, i, u);
            }
        }
    }
    else {

        char usrin[MSG_MAX], v1[MSG_MAX - CHA_MAX], v2[CHA_MAX], c1;
        struct termios tp, save;

        tcgetattr(STDIN_FILENO, &tp);
        save = tp;
        tp.c_cc[VERASE] = 127;

        if (tcsetattr(STDIN_FILENO, TCSANOW, &tp) < 0) return 2;

        while (waitpid(pid, NULL, WNOHANG) == 0) {
            if (!kbhit()) dprintf(fd[1], "/\n");
            else if (fgets(usrin, MSG_MAX, stdin) != NULL &&
                     (sscanf(usrin, "/%[m] %s %[^\n]\n", &c1, v2, v1) > 2 ||
                     sscanf(usrin, "/%[xMQqnjp] %[^\n]\n", &c1, v1) > 0)) {
                switch (c1) {
                    case 'x': dprintf(fd[1], "%s\n", v1);                   break;
                    case 'q': dprintf(fd[1], "quit\n");                     break;
                    case 'Q': dprintf(fd[1], "quit %s\n", v1);              break;
                    case 'j': dprintf(fd[1], "join %s\n", v1);              break;
                    case 'p': dprintf(fd[1], "part %s\n", v1);              break;
                    case 'n': dprintf(fd[1], "names #%s\n", chan);          break;
                    case 'M': dprintf(fd[1], "privmsg nickserv :%s\n", v1); break;
                    case 'm': dprintf(fd[1], "privmsg %s :%s\n", v2, v1);   break;
                }
            } else dprintf(fd[1], "privmsg #%s :%s", chan, usrin);
        }

        if (tcsetattr(STDIN_FILENO, TCSANOW, &save) < 0) return 2;
        puts("<< connection closed");
    }
    return 0;
}
