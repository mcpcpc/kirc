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
static char *pass = NULL;               /* server password                    */
static char *user = NULL;               /* server username                    */
static char *real = NULL;               /* real name                          */

static int
kbhit(void) {

    int    byteswaiting;
    struct termios term;
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
    if (user && real) raw("USER %s - - :%s\r\n", user, real);
    if (user && !real && nick) raw("USER %s - - :%s\r\n", user, nick);
    if (!user && !real && nick) raw("USER %s - - :%s\r\n", nick, nick);
    if (pass) raw("PASS %s\r\n", pass);

    fcntl(conn, F_SETFL, O_NONBLOCK);
}

static void
printw(const char *format, ...) {

    va_list argptr;
    char    *tok, line[BUFF + 1];
    int     len, i, s;

    va_start(argptr, format);
    vsnprintf(line, BUFF + 1, format, argptr);
    va_end(argptr);
    for (i = 0; isspace(line[i]); i++) printf("%c", line[i]);

    s = cmax + gutl - (i - 1);

    for(tok = strtok(&line[i], " "); tok != NULL; tok = strtok(NULL, " ")) {
        len = strlen(tok);
        if ((len + 1) > s) {
            printf("\n%*.s%s", gutl + 2, "", tok);
            s = cmax - (gutl + 2 + len);
        }
        else {
            printf(" %s", tok);
            s = s - (len + 1);
        }
    }
}

static void
pars(int sl, char *buf) {

    int  len, i, o = -1;
    char buf_c[BUFF + 1], ltr[200], cha[50], nic[200], hos[200], \
         usr[200], cmd[200], msg[200], pre[200];

    for (i = 0; i < sl; i++) {
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
                    len = strlen(nic);
                    printw("%*s\x1b[33;1m%-.*s\x1b[0m %s\n", \
                        gutl-(len <= gutl ? len : gutl), "", gutl, nic, msg);
                }
            }
        }
    }
}

int
main(int argc, char **argv) {

    int fd[2], cval;

    while ((cval = getopt(argc, argv, "s:p:n:k:c:u:r:w:W:vV")) != -1) {
        switch (cval) {
            case 'v' : puts("kirc 0.0.1"); break;
            case 'V' : verb = 1; break;
            case 's' : host = optarg; break;
            case 'w' : gutl = atoi(optarg); break;
            case 'W' : cmax = atoi(optarg); break;
            case 'p' : port = optarg; break;
            case 'r' : real = optarg; break;
            case 'u' : user = optarg; break;
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
        char u[BUFF];

        con();

        while ((sl = read(conn, sbuf, BUFF))) {
            pars(sl, sbuf);
            if (read(fd[0], u, BUFF) > 0) {
                for (i = 0; u[i] != '\n'; i++) continue;
                if (u[0] != ':') raw("%-*.*s\r\n", i, i, u);
            }
        }
        printf("%*s<<press RETURN key to exit>>", gutl+2, "");
    }
    else {
        char usrin[BUFF], v1[BUFF-20], v2[20], c1;
        struct termios tp, save;

        tcgetattr(STDIN_FILENO, &tp);
        save = tp;
        tp.c_cc[VERASE] = 127;

        while (waitpid(pid, NULL, WNOHANG) == 0) {
            while (!kbhit() && waitpid(pid, NULL, WNOHANG) == 0) {
                dprintf(fd[1], ":\n");
            }

            tcsetattr(STDIN_FILENO, TCSANOW, &tp);
            fgets(usrin, BUFF, stdin);
            tcsetattr(STDIN_FILENO, TCSANOW, &save);

            if (sscanf(usrin, ":%[M] %s %[^\n]\n", &c1, v2, v1) == 3 ||
                sscanf(usrin, ":%[Qnjpm] %[^\n]\n", &c1, v1) == 2 ||
                sscanf(usrin, ":%[q]\n", &c1) == 1) {
                switch (usrin[1]) {
                    case 'q': dprintf(fd[1], "quit\n"); break;
                    case 'Q': dprintf(fd[1], "quit %s\n", v1); break;
                    case 'j': dprintf(fd[1], "join %s\n", v1); break;
                    case 'p': dprintf(fd[1], "part %s\n", v1); break;
                    case 'm': dprintf(fd[1], "privmsg #%s :%s\n", chan, v1); break;
                    case 'n': dprintf(fd[1], "privmsg nickserv :%s\n", v1); break;
                    case 'M': dprintf(fd[1], "privmsg %s :%s\n", v2, v1); break;
                    case '?': break;
                }
            }
            else dprintf(fd[1], "%s", usrin);

            fflush(stdout);
        }
    }
    return 0;
}
