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

#define BUFF 512                        /* buffer size (see RFC 2812)         */
#define CMAX 92                         /* max number of columns              */
#define GUTL 10                         /* left gutter width and alignment    */

static int  conn;                       /* socket connection                  */
static char sbuf[BUFF];                 /* string buffer                      */
static int  verb  = 0;                  /* verbose output (e.g. raw stream)   */
static char *host = "irc.freenode.org"; /* irc host address                   */
static char *chan = "kisslinux";        /* channel                            */
static char *port = "6667";             /* port                               */
static char *nick = NULL;               /* nickname                           */

static int
kbhit(void) {
    int    byteswaiting;
    struct termios term;
    fd_set fds;
    struct timespec ts = {0};
    tcgetattr(0, &term);

    struct termios term2 = term;

    term2.c_lflag &= ~ICANON;
    tcsetattr(0, TCSANOW, &term2);
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    byteswaiting = pselect(1, &fds, NULL, NULL, &ts, NULL);
    tcsetattr(0, TCSANOW, &term);

    return byteswaiting > 0;
}

static void
raw(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(sbuf, BUFF, fmt, ap);
    va_end(ap);

    if (verb) printf("<< %s", sbuf);

    write(conn, sbuf, strlen(sbuf));
}

static void
con(void) {
    struct addrinfo *res, hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };

    getaddrinfo(host, port, &hints, &res);
    conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(conn, res->ai_addr, res->ai_addrlen);

    if (nick) raw("NICK %s\r\n", nick);
    if (nick) raw("USER %s - - :%s\r\n", nick, nick);

    fcntl(conn, F_SETFL, O_NONBLOCK);
}

static void
printw(const char *format, ...) {

    int     s1 = 0, s2, i, o;
    va_list argptr;
    char   line[BUFF + 1];

    va_start(argptr, format);
    vsnprintf(line, BUFF + 1, format, argptr);
    va_end(argptr);
    if (strlen(line) <= CMAX) printf("%s", line);
    else if (strlen(line) > CMAX) {
        for (i = 0; i < CMAX; i++) {
            if (line[i] == ' ') s1 = i;
            if (i == CMAX - 1) printf("%-*.*s\n", s1, s1, line);
        }
        s2 = o = s1;
        for (i = s1; line[i] != '\0'; i++) {
            if (line[i] == ' ') s2 = i;
            if ((i - o) == (CMAX - GUTL)) {
                printf("%*s %-*.*s\n", GUTL, " ", s2 - o, s2 - o, &line[o + 1]);
                o = i = s2;
            }
            else if (line[i + 1] == '\0') {
                printf("%*s %-*.*s", GUTL, " ", i - o, i - o, &line[o + 1]);
            }
        }
    }
}

static void
pars(int sl, char *buf) {

    char buf_c[BUFF + 1], ltr[200], cha[200], nic[200], hos[200], \
         usr[200], cmd[200], msg[200], pre[200];
    int  o = -1;

    for (int i = 0; i < sl; i++) {
        o++;
        buf_c[o] = buf[i];

        if ((i > 0 && buf[i] == '\n' && buf[i - 1] == '\r') || o == BUFF) {
            buf_c[o + 1] = '\0';
            o = -1;

            if (verb) printf(">> %s", buf_c);

            if (!strncmp(buf_c, "PING", 4)) {
                buf_c[1] = 'O';
                raw(buf_c);
            }

            else if (buf_c[0] == ':') {
                sscanf(buf_c, ":%[^ ] %[^:]:%[^\r]", pre, cmd, msg);
                sscanf(pre, "%[^!]!%[^@]@%s", nic, usr, hos);
                sscanf(cmd, "%[^#& ]%s", ltr, cha);

                if (!strncmp(ltr, "001", 3)) raw("JOIN #%s\r\n", chan);

                if (!strncmp(ltr, "QUIT", 4)) {
                    printw("%*.*s \x1b[34;1m%s\x1b[0m left %s\n", \
                        GUTL, GUTL, "<--", nic, cha);
                }
                else if (!strncmp(ltr, "JOIN", 4)) {
                    printw("%*.*s \x1b[32;1m%s\x1b[0m joined %s\n", \
                        GUTL, GUTL, "-->", nic, cha);
                }
                else {
                    printw("\x1b[1m%*.*s\x1b[0m %s\n", \
                        GUTL, GUTL, nic, msg);
                }
            }
        }
    }
}

int
main(int argc, char **argv) {

    int fd[2], cval;

    while ((cval = getopt(argc, argv, "s:p:n:k:c:vV")) != -1) {
        switch (cval) {
            case 'v' : printf("kirc 0.0.1\n"); break;
            case 'V' : verb = 1;               break;
            case 's' : host = optarg;          break;
            case 'p' : port = optarg;          break;
            case 'n' : nick = optarg;          break;
            case 'c' : chan = optarg;          break;
            case '?' : return 1;
        }
    }

    if (nick == NULL) {
        fprintf(stderr, "nick not specified");
        return 1;
    }

    if (pipe(fd) < 0) {
        fprintf(stderr, "pipe() failed");
        return 2;
    }

    pid_t pid = fork();

    if (pid == 0) {
        int  sl;
        char u[BUFF];

        con();

        while ((sl = read(conn, sbuf, BUFF))) {
            pars(sl, sbuf);
            if ((read(fd[0], u, CMAX) > 0) && !strstr(u, "WAIT_SIG")) {
                raw("%s\r\n", u);
            }
        }
        printf("(press <ENTER> to quit)\n");
    }
    else {
        char usrin[CMAX];
        char cmd = '\n';

        while (waitpid(pid, NULL, WNOHANG) == 0) {
            while (!kbhit() && waitpid(pid, NULL, WNOHANG) == 0) {
                write(fd[1], "WAIT_SIG", CMAX);
            }

            fgets(usrin, CMAX, stdin);

            if (usrin[0] == ':' && usrin[1]) {
                char *cmd_val = &usrin[2];
                cmd = usrin[1];

                switch (cmd) {
                    case 'q':
                        write(fd[1], "quit", sizeof("quit"));
                        break;
                    case 'm':
                        while (isspace(*cmd_val)) cmd_val++;
                        dprintf(fd[1], "privmsg #%s :%-*s", \
                            chan, CMAX - strlen(chan) - 11, cmd_val);
                        break;
                }
            }
            else {
                write(fd[1], usrin, CMAX);
            }
            fflush(stdout);
        }
    }
    return 0;
}
