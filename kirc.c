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

#define IRC_MSG_MAX   512                /* guaranteed max message length */
#define IRC_CHAN_MAX  200                /* gauranteed max channel length */

static int    conn;                      /* connection socket */
static int    verb  = 0;                 /* verbose output (e.g. raw stream) */
static int    cmax  = 80;                /* max number of chars per line */
static int    gutl  = 10;                /* max char width of left column */
static char * host = "irc.freenode.org"; /* irc host address */
static char * chan = "kisslinux";        /* channel */
static char * port = "6667";             /* server port */
static char * nick = NULL;               /* nickname */
static char * pass = NULL;               /* server password */
static char * user = NULL;               /* server user name */
static char * real = NULL;               /* server user real name */
static char * olog = NULL;               /* log irc stream parh */

/* append string to specified file path */
static void
printa(char *str) {
    FILE *out = fopen(olog, "a");
    fprintf(out, "%s", str);
    fclose(out);
}

/* wait for keyboard press to interrupt stream */
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

/* handle keyboard strokes for command input */
static char *
input_handler() {

    char *usrin = malloc(sizeof(char) * (IRC_MSG_MAX + 1));
    struct termios tp, save;

    tcgetattr(STDIN_FILENO, &tp);
    save = tp;
    tp.c_cc[VERASE] = 127;
    tcsetattr(STDIN_FILENO, TCSANOW, &tp);
    fgets(usrin, IRC_MSG_MAX, stdin);
    tcsetattr(STDIN_FILENO, TCSANOW, &save);

    return usrin;
}

/* send command to irc server */
static void
raw(char *fmt, ...) {

    va_list ap;
    char *cmd_str = malloc(sizeof(char) * (IRC_MSG_MAX + 1));

    va_start(ap, fmt);
    vsnprintf(cmd_str, IRC_MSG_MAX, fmt, ap);
    va_end(ap);

    if (verb) printf("<< %s", cmd_str);
    if (olog) printa(cmd_str);

    write(conn, cmd_str, strlen(cmd_str));
}

/* initial irc server connection */
static void
irc_init() {

    struct addrinfo *res, hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };

    getaddrinfo(host, port, &hints, &res);
    conn = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(conn, res->ai_addr, res->ai_addrlen);

    if (nick)                   raw("NICK %s\r\n", nick);
    if (user && real)           raw("USER %s - - :%s\r\n", user, real);
    if (user && !real && nick)  raw("USER %s - - :%s\r\n", user, nick);
    if (!user && !real && nick) raw("USER %s - - :%s\r\n", nick, nick);
    if (pass)                   raw("PASS %s\r\n", pass);

    fcntl(conn, F_SETFL, O_NONBLOCK);
}

/* print formatted irc stream with word wrap and hanging indent  */
static void
printw(const char *format, ...) {

    va_list argptr;
    char    *tok, line[IRC_MSG_MAX + 1];
    int     i, wordwidth, spaceleft, spacewidth = 1;

    va_start(argptr, format);
    vsnprintf(line, IRC_MSG_MAX + 1, format, argptr);
    va_end(argptr);

    if (olog) printa(line);

    for (i = 0; isspace(line[i]); i++) printf("%c", line[i]);

    spaceleft = cmax + gutl - (i - 1);

    for(tok = strtok(&line[i], " "); tok != NULL; tok = strtok(NULL, " ")) {
        wordwidth = strlen(tok);
        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\n%*.s%s", gutl + 2, "", tok);
            spaceleft = cmax - (gutl + 2 + wordwidth);
        } else {
            printf(" %s", tok);
            spaceleft = spaceleft - (wordwidth + spacewidth);
        }
    }
}

/* parse irc stream */
static void
parser(char *in) {

    int len;
    char ltr[200], cha[IRC_CHAN_MAX], nic[200], hos[200], \
         usr[200], cmd[200], msg[200], pre[200];

    cmd[0] = msg[0] = pre[0] = '\0';
    if (verb) printf(">> %s\n", in);
    if (!strncmp(in, "PING", 4)) {
        in[1] = 'O';
        raw("%s\r\n", in);
    }
    else if (in[0] == ':') {
        sscanf(in, ":%[^ ] %[^:]:%[^\r]", pre, cmd, msg);
        sscanf(pre, "%[^!]!%[^@]@%s", nic, usr, hos);
        sscanf(cmd, "%[^#& ]%s", ltr, cha);
        if (!strncmp(ltr, "001", 3)) raw("JOIN #%s\r\n", chan);
        if (!strncmp(ltr, "QUIT", 4)) {
            printw("%*.*s \x1b[34;1m%s\x1b[0m\n", gutl, gutl, "<--", nic);
        } else if (!strncmp(ltr, "JOIN", 4)) {
            printw("%*.*s \x1b[32;1m%s\x1b[0m\n", gutl, gutl, "-->", nic);
        } else {
            len = strlen(nic);
            printw("%*s\x1b[33;1m%-.*s\x1b[0m %s\n", \
                gutl-(len <= gutl ? len : gutl), "", gutl, nic, msg);
        }
    }
}

int
main(int argc, char **argv) {

    int fd[2], cval;

    while ((cval = getopt(argc, argv, "s:p:o:n:k:c:u:r:w:W:vV")) != -1) {
        switch (cval) {
            case 'v' : puts("kirc 0.0.3"); return 0;
            case 'V' : verb = 1; break;
            case 's' : host = optarg; break;
            case 'w' : gutl = atoi(optarg); break;
            case 'W' : cmax = atoi(optarg); break;
            case 'p' : port = optarg; break;
            case 'r' : real = optarg; break;
            case 'u' : user = optarg; break;
            case 'o' : olog = optarg; break;
            case 'n' : nick = optarg; break;
            case 'k' : pass = optarg; break;
            case 'c' : chan = optarg; break;
            case '?' : return 1;
        }
    }

    if (nick == NULL) {
        fprintf(stderr, "nick not specified\n");
        return 1;
    }

    if (pipe(fd) < 0) {
        fprintf(stderr, "pipe() failed\n");
        return 2;
    }

    pid_t pid = fork();

    if (pid == 0) {
        int  sl, i, o = 0;
        char u[IRC_MSG_MAX], s, b[IRC_MSG_MAX];

        irc_init();

        while ((sl = read(conn, &s, 1))) {
            if (sl > 0) b[o] = s;

            if ((o > 0 && b[o - 1] == '\r' && b[o] == '\n') || o == IRC_MSG_MAX) {
                b[o + 1] = '\0';
                parser(b);
                o = 0;
            }
            else if (sl > 0) o++;
            if (read(fd[0], u, IRC_MSG_MAX) > 0) {
                for (i = 0; u[i] != '\n'; i++) continue;
                if (u[0] != ':') raw("%-*.*s\r\n", i, i, u);
            }
        }
    }
    else {
        char usrin[IRC_MSG_MAX], v1[IRC_MSG_MAX-20], v2[20], c1;

        while (waitpid(pid, NULL, WNOHANG) == 0) {
            if (!kbhit()) dprintf(fd[1], ":\n");
            else {
                strcpy(usrin, input_handler());

                if (sscanf(usrin, ":%[M] %s %[^\n]\n", &c1, v2, v1) == 3 ||
                    sscanf(usrin, ":%[Qnjpm] %[^\n]\n", &c1, v1) == 2 ||
                    sscanf(usrin, ":%[q]\n", &c1) == 1) {
                    switch (c1) {
                        case 'q': dprintf(fd[1], "quit\n"); break;
                        case 'Q': dprintf(fd[1], "quit %s\n", v1); break;
                        case 'j': dprintf(fd[1], "join %s\n", v1); break;
                        case 'p': dprintf(fd[1], "part %s\n", v1); break;
                        case 'm': dprintf(fd[1], "privmsg #%s :%s\n", chan, v1); break;
                        case 'n': dprintf(fd[1], "privmsg nickserv :%s\n", v1);  break;
                        case 'M': dprintf(fd[1], "privmsg %s :%s\n", v2, v1);    break;
                        case '?': break;
                    }
                }
                else dprintf(fd[1], "%s", usrin);
            }
        }

        fprintf(stderr, "%*s  <<irc server connection closed>>\n", gutl, "");
    }
    return 0;
}
