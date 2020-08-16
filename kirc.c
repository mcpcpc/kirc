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

static int  conn;                       /* socket connection                  */
static char sbuf[BUFF];                 /* string buffer                      */
static int  verb  = 0;                  /* verbose output (e.g. raw stream)   */
static int  cmax  = 82;
static int  gutl  = 10;
static char *host = "irc.freenode.org"; /* irc host address                   */
static char *chan = "kisslinux";        /* channel                            */
static char *port = "6667";             /* port                               */
static char *nick = NULL;               /* nickname                           */
static char *pass = NULL;               /* password                           */

static int
kbhit(void) {

    int    byteswaiting;
    struct termios term;
    term.c_cc[VERASE] = 127;
    tcgetattr(0, &term);
    fd_set fds;
    struct timespec ts = {0};

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
    if (pass) raw("PASS %s\r\n", pass);

    fcntl(conn, F_SETFL, O_NONBLOCK);
}

static void
printw(const char *format, ...) {

    int     s1 = 0, s2, len, i;
    va_list argptr;
    char   line[BUFF + 1];

    va_start(argptr, format);
    vsnprintf(line, BUFF + 1, format, argptr);
    va_end(argptr);

    len = strlen(line);
    if (len <= cmax + gutl) printf("%s", &line[0]);
    else if (len > cmax + gutl) {

        for (i = gutl; i < cmax + gutl; i++) {
            if (isspace(line[i])) s1 = i;
            if (i == cmax + gutl - 1) printf("%-*.*s\n", s1, s1, &line[0]);
        }

        s2 = s1;

        for (i = s1; line[i] != '\0'; i++) {
            if (isspace(line[i])) s2 = i;
            if ((i - s1) == (cmax - gutl)) {
                printf("%*s %-*.*s\n", gutl, "", s2 - s1, s2 - s1, &line[s1 + 1]);
                s1 = s2;
            }
            else if (line[i + 1] == '\0') {
                printf("%*s %-*.*s", gutl, "", i - s1, i - s1, &line[s1 + 1]);
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
                    printw("%*.*s \x1b[34;1m%s\x1b[0m\n", gutl, gutl, "<--", nic);
                }
                else if (!strncmp(ltr, "JOIN", 4)) {
                    printw("%*.*s \x1b[32;1m%s\x1b[0m\n", gutl, gutl, "-->", nic);
                }
                else {
                    printw("\x1b[33;1m%*.*s\x1b[0m %s\n", gutl, gutl, nic, msg);
                }
            }
        }
    }
}

int
main(int argc, char **argv) {

    int fd[2], cval;


    while ((cval = getopt(argc, argv, "s:p:n:k:c:w:W:vV")) != -1) {
        switch (cval) {
            case 'v' : puts("kirc 0.0.1"); break;
            case 'V' : verb = 1; break;
            case 's' : host = optarg; break;
            case 'w' : gutl = atoi(optarg); break;
            case 'W' : cmax = atoi(optarg); break;
            case 'p' : port = optarg; break;
            case 'n' : nick = optarg; break;
            case 'k' : pass = optarg; break;
            case 'c' : chan = optarg; break;
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
        int  sl, i;
        char u[cmax];

        con();

        while ((sl = read(conn, sbuf, BUFF))) {
            pars(sl, sbuf);
            if (read(fd[0], u, cmax) > 0) {
                for (i = 0; u[i] != '\n'; i++) continue;
                if (u[0] != ':') {
				    raw("%-*.*s\r\n", i, i, u);
                    printf("\x1b[1A\x1b[K\x1b[36m%-*.*s\x1b[0m\n", i, i, u);
				}
            }
        }
        printf("%*s \x1b[31mpress <RETURN> key to quit...\x1b[0m", gutl, " ");
    }
    else {
        char usrin[cmax], val1[cmax], val2[20];

        while (waitpid(pid, NULL, WNOHANG) == 0) {
            while (!kbhit() && waitpid(pid, NULL, WNOHANG) == 0) {
                dprintf(fd[1], ":\n");
            }

            fgets(usrin, cmax, stdin);

            if (usrin[0] == ':') {
                switch (usrin[1]) {
                    case 'q':
                        dprintf(fd[1],"quit\n");
                        break;
                    case 'm':
                        sscanf(usrin, ":m %[^\n]\n", val1);
                        dprintf(fd[1], "privmsg #%s :%s\n", chan, val1);
                        break;
                    case 'n':
                        sscanf(usrin, ":n %[^\n]\n", val1);
                        dprintf(fd[1], "privmsg nickserv :%s\n", val1);
                        break;
                    case 'j':
                        sscanf(usrin, ":j %[^\n]\n", val1);
                        dprintf(fd[1], "join %s\n", val1);
                        break;
                    case 'p':
                        sscanf(usrin, ":p %[^\n]\n", val1);
                        dprintf(fd[1], "part %s\n", val1);
                        break;
                    case 'M':
                        sscanf(usrin, ":M %s %[^\n]\n", val2, val1);
                        dprintf(fd[1], "privmsg %s :%s\n", val2, val1);
                        break;
                    case '\n':
                        break;
                    default:
                        printf("\x1b[31munknown ':%c' \x1b[0m\n", usrin[1]);
                        break;
                }
            }
            else {
                dprintf(fd[1], "%s", usrin);
            }
            fflush(stdout);
        }
    }
    return 0;
}
