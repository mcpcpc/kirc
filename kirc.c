#include "kirc.h"

/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2023 Michael Czigler
 */

static void free_history(void)
{
    if (history) {
        int j;
        for (j = 0; j < history_len; j++) {
            free(history[j]);
        }
        free(history);
    }
}

static void disable_raw_mode(void)
{
    if (rawmode && tcsetattr(ttyinfd, TCSAFLUSH, &orig) != -1) {
        rawmode = 0;
    }
    free_history();
}

static int enable_raw_mode(int fd)
{
    if (!isatty(ttyinfd)) {
        goto fatal;
    }
    if (!atexit_registered) {
        atexit(disable_raw_mode);
        atexit_registered = 1;
    }
    if (tcgetattr(fd, &orig) == -1) {
        goto fatal;
    }
    struct termios raw = orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) {
        goto fatal;
    }
    rawmode = 1;
    return 0;
 fatal:
    errno = ENOTTY;
    return -1;
}

static int get_cursor_position(int ifd, int ofd)
{
    char buf[32];
    int cols, rows;
    unsigned int i = 0;
    if (write(ofd, "\x1b[6n", 4) != 4) {
        return -1;
    }
    while (i < sizeof(buf) - 1) {
        if (read(ifd, buf + i, 1) != 1) {
            break;
        }
        if (buf[i] == 'R') {
            break;
        }
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != 27 || buf[1] != '[') {
        return -1;
    }
    if (sscanf(buf + 2, "%d;%d", &rows, &cols) != 2) {
        return -1;
    }
    return cols;
}

static int get_columns(int ifd, int ofd)
{
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        int start = get_cursor_position(ifd, ofd);
        if (start == -1) {
            return 80;
        }
        if (write(ofd, "\x1b[999C", 6) != 6) {
            return 80;
        }
        int cols = get_cursor_position(ifd, ofd);
        if (cols == -1) {
            return 80;
        }
        if (cols > start) {
            char seq[32];
            snprintf(seq, sizeof(seq), "\x1b[%dD", cols - start);
            if (write(ofd, seq, strnlen(seq, MSG_MAX)) == -1) {
            }
        }
        return cols;
    } else {
        return ws.ws_col;
    }
}

static void
buffer_position_move(state l, ssize_t dest, ssize_t src, size_t size)
{
    memmove(l->buf + l->posb + dest, l->buf + l->posb + src, size);
}

static void buffer_position_move_end(state l, ssize_t dest, ssize_t src)
{
    buffer_position_move(l, dest, src, l->lenb - (l->posb + src) + 1);
}

static int u8_character_start(char c)
{
    int ret = 1;
    if (isu8 != 0) {
        ret = (c & 0x80) == 0x00 || (c & 0xC0) == 0xC0;
    }
    return ret;
}

static int u8_character_size(char c)
{
    int ret = 1;
    if (isu8 != 0) {
        int size = 0;
        while (c & (0x80 >> size)) {
            size++;
        }
        ret = (size != 0) ? size : 1;
    }
    return ret;
}

static size_t u8_length(const char *s)
{
    size_t lenu8 = 0;
    while (*s != '\0') {
        lenu8 += u8_character_start(*(s++));
    }
    return lenu8;
}

static size_t u8_previous(const char *s, size_t posb)
{
    if (posb != 0) {
        do {
            posb--;
        } while ((posb > 0) && !u8_character_start(s[posb]));
    }
    return posb;
}

static size_t u8_next(const char *s, size_t posb)
{
    if (s[posb] != '\0') {
        do {
            posb++;
        } while ((s[posb] != '\0') && !u8_character_start(s[posb]));
    }
    return posb;
}

static int setIsu8_C(int ifd, int ofd)
{
    if (isu8) {
        return 0;
    }
    if (write(ofd, "\r", 1) != 1) {
        return -1;
    }
    if (get_cursor_position(ifd, ofd) != 1) {
        return -1;
    }
    const char *testChars[] = {
        "\xe1\xbb\xa4",
        NULL
    };
    for (const char **it = testChars; *it; it++) {
        if (write(ofd, *it, strlen(*it)) != (ssize_t) strlen(*it)) {
            return -1;
        }
        int pos = get_cursor_position(ifd, ofd);
        if (write(ofd, "\r", 1) != 1) {
            return -1;
        }
        for (int i = 1; i < pos; i++) {
            if (write(ofd, " ", 1) != 1) {
                return -1;
            }
        }
        if (write(ofd, "\r", 1) != 1) {
            return -1;
        }
        if (pos != 2) {
            return 0;
        }
    }
    isu8 = 1;
    return 0;
}

static void ab_initialize(struct abuf *ab)
{
    ab->b = NULL;
    ab->len = 0;
}

static void ab_append(struct abuf *ab, const char *s, int len)
{
    char *new = realloc(ab->b, ab->len + len);
    if (new == NULL) {
        return;
    }
    memcpy(new + ab->len, s, len);
    ab->b = new;
    ab->len += len;
}

static void ab_free(struct abuf *ab)
{
    free(ab->b);
}

static void refresh_line(state l)
{
    char seq[64];
    size_t plenu8 = l->plenu8 + 2;
    int fd = STDOUT_FILENO;
    char *buf = l->buf;
    size_t lenb = l->lenb;
    size_t posu8 = l->posu8;
    size_t ch = plenu8, txtlenb = 0;
    struct abuf ab;
    l->cols = get_columns(ttyinfd, STDOUT_FILENO);
    while ((plenu8 + posu8) >= l->cols) {
        buf += u8_next(buf, 0);
        posu8--;
    }
    while (txtlenb < lenb && ch++ < l->cols)
        txtlenb += u8_next(buf + txtlenb, 0);
    ab_initialize(&ab);
    snprintf(seq, sizeof(seq), "\r");
    ab_append(&ab, seq, strnlen(seq, MSG_MAX));
    ab_append(&ab, l->prompt, l->plenb);
    ab_append(&ab, "> ", 2);
    ab_append(&ab, buf, txtlenb);
    snprintf(seq, sizeof(seq), "\x1b[0K");
    ab_append(&ab, seq, strnlen(seq, MSG_MAX));
    if (posu8 + plenu8) {
        snprintf(seq, sizeof(seq), "\r\x1b[%dC", (int)(posu8 + plenu8));
    } else {
        snprintf(seq, sizeof(seq), "\r");
    }
    ab_append(&ab, seq, strnlen(seq, MSG_MAX));
    if (write(fd, ab.b, ab.len) == -1) {
    }
    ab_free(&ab);
}

static int edit_insert(state l, char *c)
{
    size_t clenb = strlen(c);
    if ((l->lenb + clenb) < l->buflen) {
        if (l->lenu8 == l->posu8) {
            strcpy(l->buf + l->posb, c);
            l->posu8++;
            l->lenu8++;
            l->posb += clenb;
            l->lenb += clenb;
            if ((l->plenu8 + l->lenu8) < l->cols) {
                if (write(STDOUT_FILENO, c, clenb) == -1) {
                    return -1;
                }
            } else {
                refresh_line(l);
            }
        } else {
            buffer_position_move_end(l, clenb, 0);
            memmove(l->buf + l->posb, c, clenb);
            l->posu8++;
            l->lenu8++;
            l->posb += clenb;
            l->lenb += clenb;
            refresh_line(l);
        }
    }
    return 0;
}

static void edit_move_left(state l)
{
    if (l->posb > 0) {
        l->posb = u8_previous(l->buf, l->posb);
        l->posu8--;
        refresh_line(l);
    }
}

static void edit_move_right(state l)
{
    if (l->posu8 != l->lenu8) {
        l->posb = u8_next(l->buf, l->posb);
        l->posu8++;
        refresh_line(l);
    }
}

static void edit_move_home(state l)
{
    if (l->posb != 0) {
        l->posb = 0;
        l->posu8 = 0;
        refresh_line(l);
    }
}

static void edit_move_end(state l)
{
    if (l->posu8 != l->lenu8) {
        l->posb = l->lenb;
        l->posu8 = l->lenu8;
        refresh_line(l);
    }
}

static void edit_delete(state l)
{
    if ((l->lenu8 > 0) && (l->posu8 < l->lenu8)) {
        size_t this_size = u8_next(l->buf, l->posb) - l->posb;
        buffer_position_move_end(l, 0, this_size);
        l->lenb -= this_size;
        l->lenu8--;
        refresh_line(l);
    }
}

static void edit_backspace(state l)
{
    if ((l->posu8 > 0) && (l->lenu8 > 0)) {
        size_t prev_size = l->posb - u8_previous(l->buf, l->posb);
        buffer_position_move_end(l, (ssize_t) - prev_size, 0);
        l->posb -= prev_size;
        l->lenb -= prev_size;
        l->posu8--;
        l->lenu8--;
        refresh_line(l);
    }
}

static void edit_delete_previous_word(state l)
{
    size_t old_posb = l->posb;
    size_t old_posu8 = l->posu8;
    while ((l->posb > 0) && (l->buf[l->posb - 1] == ' ')) {
        l->posb--;
        l->posu8--;
    }
    while ((l->posb > 0) && (l->buf[l->posb - 1] != ' ')) {
        if (u8_character_start(l->buf[l->posb - 1])) {
            l->posu8--;
        }
        l->posb--;
    }
    size_t diffb = old_posb - l->posb;
    size_t diffu8 = old_posu8 - l->posu8;
    buffer_position_move_end(l, 0, diffb);
    l->lenb -= diffb;
    l->lenu8 -= diffu8;
    refresh_line(l);
}

static void edit_delete_whole_line(state l)
{
    l->buf[0] = '\0';
    l->posb = l->lenb = l->posu8 = l->lenu8 = 0;
    refresh_line(l);
}

static void edit_delete_line_to_end(state l)
{
    l->buf[l->posb] = '\0';
    l->lenb = l->posb;
    l->lenu8 = l->posu8;
    refresh_line(l);
}

static void edit_swap_character_w_previous(state l)
{
    if (l->posu8 > 0 && l->posu8 < l->lenu8) {
        char aux[8];
        ssize_t prev_size = l->posb - u8_previous(l->buf, l->posb);
        ssize_t this_size = u8_next(l->buf, l->posb) - l->posb;
        memmove(aux, l->buf + l->posb, this_size);
        buffer_position_move(l, -prev_size + this_size, -prev_size, prev_size);
        memmove(l->buf + l->posb - prev_size, aux, this_size);
        if (l->posu8 != l->lenu8 - 1) {
            l->posu8++;
            l->posb += this_size;
        }
        refresh_line(l);
    }
}

static void edit_history(state l, int dir)
{
    if (history_len > 1) {
        free(history[history_len - (1 + l->history_index)]);
        history[history_len - (1 + l->history_index)] = strdup(l->buf);
        l->history_index += (dir == 1) ? 1 : -1;
        if (l->history_index < 0) {
            l->history_index = 0;
            return;
        } else if (l->history_index >= history_len) {
            l->history_index = history_len - 1;
            return;
        }
        strncpy(l->buf, history[history_len - (1 + l->history_index)],
                l->buflen);
        l->buf[l->buflen - 1] = '\0';
        l->lenb = l->posb = strnlen(l->buf, MSG_MAX);
        l->lenu8 = l->posu8 = u8_length(l->buf);
        refresh_line(l);
    }
}

static int history_add(const char *line)
{
    char *linecopy;
    if (history_max_len == 0) {
        return 0;
    }
    if (history == NULL) {
        history = malloc(sizeof(char *) * history_max_len);
        if (history == NULL) {
            return 0;
        }
        memset(history, 0, (sizeof(char *) * history_max_len));
    }
    if (history_len && !strcmp(history[history_len - 1], line)) {
        return 0;
    }
    linecopy = strdup(line);
    if (!linecopy) {
        return 0;
    }
    if (history_len == history_max_len) {
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (history_max_len - 1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

static void edit_enter(void)
{
    history_len--;
    free(history[history_len]);
}

static void edit_escape_sequence(state l, char seq[3])
{
    if (read(ttyinfd, seq, 1) == -1)
        return;
    if (read(ttyinfd, seq + 1, 1) == -1)
        return;
    if (seq[0] == '[') {        /* ESC [ sequences. */
        if ((seq[1] >= '0') && (seq[1] <= '9')) {
            /* Extended escape, read additional byte. */
            if (read(ttyinfd, seq + 2, 1) == -1) {
                return;
            }
            if ((seq[2] == '~') && (seq[1] == 3)) {
                edit_delete(l); /* Delete key. */
            }
        } else {
            switch (seq[1]) {
            case 'A':
                edit_history(l, 1);
                break;          /* Up */
            case 'B':
                edit_history(l, 0);
                break;          /* Down */
            case 'C':
                edit_move_right(l);
                break;          /* Right */
            case 'D':
                edit_move_left(l);
                break;          /* Left */
            case 'H':
                edit_move_home(l);
                break;          /* Home */
            case 'F':
                edit_move_end(l);
                break;          /* End */
            }
        }
    } else if (seq[0] == 'O') { /* ESC O sequences. */
        switch (seq[1]) {
        case 'H':
            edit_move_home(l);
            break;              /* Home */
        case 'F':
            edit_move_end(l);
            break;              /* End */
        }
    }
}

static int edit(state l)
{
    char c, seq[3];
    int ret = 0;
    ssize_t nread = read(ttyinfd, &c, 1);
    if (nread <= 0) {
        ret = 1;
    } else {
        switch (c) {
        case 13:
            edit_enter();
            return 1;           /* enter */
        case 3:
            errno = EAGAIN;
            return -1;          /* ctrl-c */
        case 127:              /* backspace */
        case 8:
            edit_backspace(l);
            break;              /* ctrl-h */
        case 2:
            edit_move_left(l);
            break;              /* ctrl-b */
        case 6:
            edit_move_right(l);
            break;              /* ctrl-f */
        case 1:
            edit_move_home(l);
            break;              /* Ctrl+a */
        case 5:
            edit_move_end(l);
            break;              /* ctrl+e */
        case 23:
            edit_delete_previous_word(l);
            break;              /* ctrl+w */
        case 21:
            edit_delete_whole_line(l);
            break;              /* Ctrl+u */
        case 11:
            edit_delete_line_to_end(l);
            break;              /* Ctrl+k */
        case 14:
            edit_history(l, 0);
            break;              /* Ctrl+n */
        case 16:
            edit_history(l, 1);
            break;              /* Ctrl+p */
        case 20:
            edit_swap_character_w_previous(l);
            break;              /* ctrl-t */
        case 27:
            edit_escape_sequence(l, seq);
            break;              /* escape sequence */
        case 4:                /* ctrl-d */
            if (l->lenu8 > 0) {
                edit_delete(l);
            } else {
                history_len--;
                free(history[history_len]);
                ret = -1;
            }
            break;
        default:
            if (u8_character_start(c)) {
                char aux[8];
                aux[0] = c;
                int size = u8_character_size(c);
                for (int i = 1; i < size; i++) {
                    nread = read(ttyinfd, aux + i, 1);
                    if ((aux[i] & 0xC0) != 0x80) {
                        break;
                    }
                }
                aux[size] = '\0';
                if (edit_insert(l, aux)) {
                    ret = -1;
                }
            }
            break;
        }
    }
    return ret;
}

static void state_reset(state l)
{
    l->plenb = strnlen(l->prompt, MSG_MAX);
    l->plenu8 = u8_length(l->prompt);
    l->oldposb = l->posb = l->oldposu8 = l->posu8 = l->lenb = l->lenu8 = 0;
    l->history_index = 0;
    l->buf[0] = '\0';
    l->buflen--;
    history_add("");
}

static char *ctime_now(char buf[26])
{
    struct tm tm_;
    time_t now = time(NULL);
    if (!asctime_r(localtime_r(&now, &tm_), buf)) {
        return NULL;
    }
    *strchr(buf, '\n') = '\0';
    return buf;
}

static void log_append(char *str, char *path)
{
    FILE *out;
    char buf[26];
    if ((out = fopen(path, "a")) == NULL) {
        perror("fopen");
        exit(1);
    }
    ctime_now(buf);
    fprintf(out, "%s:", buf);
    while (*str != '\0') {
        if (*str >= 32 && *str < 127) {
            fwrite(str, sizeof(char), 1, out);
        } else if (*str == 3 || *str == 4) {
            str++;
        } else if (*str == '\n') {
            fwrite("\n", sizeof(char), 1, out);
        }
        str++;
    };
    fclose(out);
}

static void raw(char *fmt, ...)
{
    va_list ap;
    char *cmd_str = malloc(MSG_MAX);
    if (!cmd_str) {
        perror("malloc");
        exit(1);
    }
    va_start(ap, fmt);
    vsnprintf(cmd_str, MSG_MAX, fmt, ap);
    va_end(ap);
    if (verb) {
        printf("<< %s", cmd_str);
    }
    if (olog) {
        log_append(cmd_str, olog);
    }
    if (write(conn, cmd_str, strnlen(cmd_str, MSG_MAX)) < 0) {
        perror("write");
        exit(1);
    }
    free(cmd_str);
}

static int connection_initialize(void)
{
    int gai_status;
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

static void message_wrap(param p)
{
    if (!p->message) {
        return;
    }
    char *tok;
    size_t wordwidth, spacewidth = 1;
    size_t spaceleft = p->maxcols - (p->nicklen + p->offset);
    for (tok = strtok(p->message, " "); tok != NULL; tok = strtok(NULL, " ")) {
        wordwidth = strnlen(tok, MSG_MAX);
        if ((wordwidth + spacewidth) > spaceleft) {
            printf("\r\n%*.s%s ", (int)p->nicklen + 1, " ", tok);
            spaceleft = p->maxcols - (p->nicklen + 1);
        } else {
            printf("%s ", tok);
        }
        spaceleft -= wordwidth + spacewidth;
    }
}

static void param_print_nick(param p)
{
    printf("\x1b[35;1m%*s\x1b[0m ", p->nicklen - 4, p->nickname);
    printf("--> \x1b[35;1m%s\x1b[0m", p->message);
}

static void param_print_part(param p)
{
    printf("%*s<-- \x1b[34;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
    if (p->channel != NULL && strcmp(p->channel + 1, cdef)) {
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
    }
}

static void param_print_quit(param p)
{
    printf("%*s<<< \x1b[34;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
}

static void param_print_join(param p)
{
    printf("%*s--> \x1b[32;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
    if (p->channel != NULL && strcmp(p->channel + 1, cdef)) {
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
    }
}

/* TODO: since we don't have config files how do we configure a download directory? */
static void handle_dcc(param p)
{
    const char *message = p->message + 5;
    if (!strncmp(message, "SEND", 4)) {
        char filename[FNM_MAX];
        size_t file_size = 0;
        unsigned ip_addr = 0;
        unsigned short port = 0;

        /* TODO: resumption of downloads */

        /* TODO: the file size parameter is optional so this isn't strictly correct.
           furthermore, during testing i've seen XDCC bots such as iroffer send ipv6
           addresses as well as ipv4 ones; however, i have yet to see that in the wild
           from what i can tell other general purpose irc clients like irssi don't try
           handle that case either.
        */
        if (sscanf(message, "SEND \"%255[^\"]\" %u %hu %zu", filename, &ip_addr, &port, &file_size) != 4) {
            if (sscanf(message, "SEND %255s %u %hu %zu", filename, &ip_addr, &port, &file_size) != 4) {
                /* TODO: i'm not quite sure how we want to handle showing error messages to the user */
                printf("error reading dcc message: '%s'\r\n", message);
                return;
            }
        }

        int slot = -1;
        /* check for an open slot */
        for (int i = 0; i < CON_MAX; i++) {
            if (dcc_sessions.sock_fds[i].fd < 0) {
                slot = i;
                break;
            }
        }

        if (slot < 0) {
            return;
        }

        /* TODO: at this point we should make sure that the filename is actually a filename
           and not a file path. furthermore, we should it would be helpful to give the user
           the option to rename to the file.
        */

        struct sockaddr_in sockaddr = (struct sockaddr_in){
            .sin_family = AF_INET,
            .sin_addr = (struct in_addr){htonl(ip_addr)},
            .sin_port = htons(port),
        };

        /* TODO: error handling */
        int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd < 0) {
            perror("socket");
            exit(1);
        }

        if (connect(sock_fd, (const struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
            perror("connect");
            close(sock_fd);
            exit(1);
        }

        int flags = fcntl(sock_fd, F_GETFL, 0) | O_NONBLOCK;
        fcntl(sock_fd, F_SETFL, flags);

        int file_fd = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (file_fd < 0) {
            perror("open");
            close(sock_fd);
            exit(1);
        }

        dcc_sessions.sock_fds[slot].fd = sock_fd;
        dcc_sessions.file_fds[slot] = file_fd;
        dcc_sessions.file_size[slot] = file_size;
        dcc_sessions.bytes_read[slot] = 0;
    }
}

static void handle_ctcp(param p)
{
    if (p->message[0] != '\001' && strncmp(p->message, "ACTION", 6)) {
        return;
    }
    const char *message = p->message + 1;
    if (!strncmp(message, "VERSION", 7)) {
        raw("NOTICE %s :\001VERSION kirc " VERSION "\001\r\n", p->nickname);
    } else if (!strncmp(message, "TIME", 7)) {
        char buf[26];
        if (!ctime_now(buf)) {
            raw("NOTICE %s :\001TIME %s\001\r\n", p->nickname, buf);
        }
    } else if (!strncmp(message, "CLIENTINFO", 10)) {
        raw("NOTICE %s :\001CLIENTINFO " CTCP_CMDS "\001\r\n", p->nickname);
    } else if (!strncmp(message, "PING", 4)) {
        raw("NOTICE %s :\001%s\r\n", p->nickname, message);
    } else if (!strncmp(message, "DCC", 3)) {
        handle_dcc(p);
    }
}

static void param_print_private(param p)
{
    int s = 0;
    if (strnlen(p->nickname, MSG_MAX) <= (size_t)p->nicklen) {
        s = p->nicklen - strnlen(p->nickname, MSG_MAX);
    }
    if (p->channel != NULL && (strcmp(p->channel, nick) == 0)) {
        handle_ctcp(p);
        printf("%*s\x1b[33;1m%-.*s\x1b[36m ", s, "", p->nicklen, p->nickname);
    } else if (p->channel != NULL && strcmp(p->channel + 1, cdef)) {
        printf("%*s\x1b[33;1m%-.*s\x1b[0m", s, "", p->nicklen, p->nickname);
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
        p->offset += 12 + strnlen(p->channel, CHA_MAX);
    } else {
        printf("%*s\x1b[33;1m%-.*s\x1b[0m ", s, "", p->nicklen, p->nickname);
    }
    if (!strncmp(p->message, "\x01" "ACTION", 7)) {
        p->message += 7;
        p->offset += 10;
        printf("[ACTION] ");
    }
}

static void param_print_channel(param p)
{
    int s = 0;
    if (strnlen(p->nickname, MSG_MAX) <= (size_t)p->nicklen) {
        s = p->nicklen - strnlen(p->nickname, MSG_MAX);
    }
    printf("%*s\x1b[33;1m%-.*s\x1b[0m ", s, "", p->nicklen, p->nickname);
    if (p->params) {
        printf("%s", p->params);
        p->offset += strnlen(p->params, CHA_MAX);
    }
}

static void raw_parser(char *string)
{
    if (!strncmp(string, "PING", 4)) {
        string[1] = 'O';
        raw("%s\r\n", string);
        return;
    }
    if (string[0] != ':' || (strnlen(string, MSG_MAX) < 4)) {
        return;
    }
    printf("\r\x1b[0K");
    if (verb) {
        printf(">> %s", string);
    }
    if (olog) {
        log_append(string, olog);
    }
    char *tok;
    param_t p = {
        .prefix = strtok(string, " ") + 1,
        .suffix = strtok(NULL, ":"),
        .message = strtok(NULL, "\r"),
        .nickname = strtok(p.prefix, "!"),
        .command = strtok(p.suffix, "#& "),
        .channel = strtok(NULL, " \r"),
        .params = strtok(NULL, ":\r"),
        .maxcols = get_columns(ttyinfd, STDOUT_FILENO),
        .nicklen = (p.maxcols / 3 > NIC_MAX ? NIC_MAX : p.maxcols / 3),
        .offset = 0
    };
    if (!strncmp(p.command, "001", 3) && chan != NULL) {
        for (tok = strtok(chan, ",|"); tok != NULL; tok = strtok(NULL, ",|")) {
            strcpy(cdef, tok);
            raw("JOIN #%s\r\n", tok);
        }
        return;
    } else if (!strncmp(p.command, "QUIT", 4)) {
        param_print_quit(&p);
    } else if (!strncmp(p.command, "PART", 4)) {
        param_print_part(&p);
    } else if (!strncmp(p.command, "JOIN", 4)) {
        param_print_join(&p);
    } else if (!strncmp(p.command, "NICK", 4)) {
        param_print_nick(&p);
    } else if (!strncmp(p.command, "PRIVMSG", 7)) {
        param_print_private(&p);
        message_wrap(&p);
    } else {
        param_print_channel(&p);
        message_wrap(&p);
    }
    printf("\x1b[0m\r\n");
}

static char message_buffer[MSG_MAX + 1];
static size_t message_end = 0;

static int handle_server_message(void)
{
    for (;;) {
        ssize_t nread = read(conn, &message_buffer[message_end],
                             MSG_MAX - message_end);
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
            if (i != 0 && message_buffer[i - 1] == '\r'
                && message_buffer[i] == '\n') {
                char saved_char = message_buffer[i + 1];
                message_buffer[i + 1] = '\0';
                raw_parser(message_buffer);
                message_buffer[i + 1] = saved_char;
                memmove(&message_buffer, &message_buffer[i + 1],
                        message_end - i - 1);
                message_end = message_end - i - 1;
                i = 0;
            }
        }
        if (message_end == MSG_MAX) {
            message_end = 0;
        }
    }
}

static void handle_user_input(state l)
{
    if (l->buf == NULL) {
        return;
    }
    char *tok;
    size_t msg_len = strnlen(l->buf, MSG_MAX);
    if (msg_len > 0 && l->buf[msg_len - 1] == '\n') {
        l->buf[msg_len - 1] = '\0';
    }
    printf("\r\x1b[0K");
    switch (l->buf[0]) {
    case '/':{
		/* send system command */
	if(l->buf[1]=='/'){
                raw("privmsg #%s :%s\r\n", l->prompt, l->buf + 3);
                printf("\x1b[35mprivmsg #%s :%s\x1b[0m\r\n", l->prompt, l->buf + 3);
	}
	else
	if(!strncmp(l->buf+1, "MSG", 3)||!strncmp(l->buf+1, "msg", 3)){
		strtok_r(l->buf + 5, " ", &tok);
		if(*(tok+strlen(tok)+1))
		*(tok+strlen(tok))=' ';
		raw("privmsg %s :%s\r\n", l->buf + 5, tok);
		printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", l->buf + 5, tok);
	}
	else
        if (l->buf[1] == '#') {
            strcpy(cdef, l->buf + 2);
            printf("\x1b[35mnew channel: #%s\x1b[0m\r\n", cdef);
        } else {
            raw("%s\r\n", l->buf + 1);
            printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
        }
	}
        break;
    case '@':                  /* send private message to target channel or user */
        strtok_r(l->buf, " ", &tok);
        if (l->buf[1] == '@') {
            if (l->buf[2] == '\0') {
                raw("privmsg #%s :\001ACTION %s\001\r\n", cdef, tok);
                printf("\x1b[35mprivmsg #%s :ACTION %s\x1b[0m\r\n", cdef, tok);
            } else {
                raw("privmsg %s :\001ACTION %s\001\r\n", l->buf + 2, tok);
                printf("\x1b[35mprivmsg %s :ACTION %s\x1b[0m\r\n", l->buf + 2,
                       tok);
            }
        } else {
            raw("privmsg %s :%s\r\n", l->buf + 1, tok);
            printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", l->buf + 1, tok);
        }
        break;
    default:                   /*  send private message to default channel */
        raw("privmsg #%s :%s\r\n", cdef, l->buf);
        printf("\x1b[35mprivmsg #%s :%s\x1b[0m\r\n", cdef, l->buf);
    }
}

static unsigned long long htonll(unsigned long long x) {
    union { int i; char c; } u = { 1 };
    return u.c ? ((unsigned long long)htonl(x & 0xFFFFFFFF) << 32) | htonl(x >> 32) : x;
}

static void usage(void)
{
    fputs("kirc [-s host] [-p port] [-c channel] [-n nick] [-r realname] \
[-u username] [-k password] [-a token] [-o path] [-e] [-x] [-v] [-V]\n", stderr);
    exit(2);
}

static void version(void)
{
    fputs("kirc-" VERSION " Copyright © 2022 Michael Czigler, MIT License\n",
          stdout);
    exit(0);
}

static void open_tty()
{
    if ((ttyinfd = open("/dev/tty", 0)) == -1) {
        perror("failed to open /dev/tty");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    int check = 0;
    char buf[BUFSIZ];
    open_tty();
    int cval;
    while ((cval = getopt(argc, argv, "s:p:o:n:k:c:u:r:a:exvV")) != -1) {
        switch (cval) {
        case 'v':
            version();
            break;
        case 'V':
            ++verb;
            break;
        case 'e':
            ++sasl;
            break;
        case 's':
            host = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case 'r':
            real = optarg;
            break;
        case 'u':
            user = optarg;
            break;
        case 'a':
            auth = optarg;
            break;
        case 'o':
            olog = optarg;
            break;
        case 'n':
            nick = optarg;
            break;
        case 'k':
            pass = optarg;
            break;
        case 'c':
            chan = optarg;
	    check = 1;
            break;
        case 'x':
            cmds = 1;
            inic = argv[optind];
            break;
        case '?':
            usage();
            break;
        }
    }
    if (cmds > 0) {
        int flag = 0;
        for (int i = 0; i < CBUF_SIZ && flag > -1; i++) {
            flag = read(STDIN_FILENO, &cbuf[i], 1);
        }
    }
    if (!nick) {
        fputs("Nick not specified\n", stderr);
        usage();
    }
    if (connection_initialize() != 0) {
        return 1;
    }
    if (auth || sasl) {
        raw("CAP REQ :sasl\r\n");
    }
    raw("NICK %s\r\n", nick);
    raw("USER %s - - :%s\r\n", (user ? user : nick), (real ? real : nick));
    if (auth || sasl) {
        raw("AUTHENTICATE %s\r\n", (sasl ? "EXTERNAL" : "PLAIN"));
        raw("AUTHENTICATE %s\r\nCAP END\r\n", (sasl ? "+" : auth));
    }
    if (pass) {
        raw("PASS %s\r\n", pass);
    }
    if (cmds > 0) {
        for (char *tok = strtok(cbuf, "\n"); tok; tok = strtok(NULL, "\n")) {
            raw("%s\r\n", tok);
        }
        if (inic) {
            raw("%s\r\n", inic);
        }
    }
    for (int i = 0; i < CON_MAX; i++) {
        dcc_sessions.sock_fds[i] = (struct pollfd){.fd = -1, .events = POLLIN | POLLOUT};
        dcc_sessions.file_fds[i] = -1;
        dcc_sessions.file_size[i] = 0;
        dcc_sessions.bytes_read[i] = 0;
    }
    dcc_sessions.sock_fds[CON_MAX] = (struct pollfd){.fd = ttyinfd,.events = POLLIN};
    dcc_sessions.sock_fds[CON_MAX + 1] = (struct pollfd){.fd = conn,.events = POLLIN};
    char usrin[MSG_MAX];
    state_t l = {
        .buf = usrin,
        .buflen = MSG_MAX,
        .prompt = cdef
    };
    if(check)
	l.prompt = chan;
    state_reset(&l);
    int rc, editReturnFlag = 0;
    if (enable_raw_mode(ttyinfd) == -1) {
        return 1;
    }
    if (setIsu8_C(ttyinfd, STDOUT_FILENO) == -1) {
        return 1;
    }
    for (;;) {
        if (poll(dcc_sessions.sock_fds, CON_MAX + 2, -1) != -1) {
            if (dcc_sessions.sock_fds[CON_MAX + 0].revents & POLLIN) {
                editReturnFlag = edit(&l);
                if (editReturnFlag > 0) {
                    history_add(l.buf);
                    handle_user_input(&l);
                    state_reset(&l);
                } else if (editReturnFlag < 0) {
                    puts("\r\n");
                    return 0;
                }
                refresh_line(&l);
            }
            if (dcc_sessions.sock_fds[CON_MAX + 1].revents & POLLIN) {
                rc = handle_server_message();
                if (rc != 0) {
                    if (rc == -2) {
                        return 1;
                    }
                    return 0;
                }
                refresh_line(&l);
            }

            /* TODO: implicitly assumes that the size reported was the correct size */
            for (int i = 0; i < CON_MAX; i++) {
                if (dcc_sessions.sock_fds[i].revents & POLLIN) {
                    int n = read(dcc_sessions.sock_fds[i].fd, buf, BUFSIZ);
                    if (n > 0) {
                        dcc_sessions.bytes_read[i] += n;
                        write(dcc_sessions.file_fds[i], buf, n);
                    }

                    if (dcc_sessions.sock_fds[i].revents & POLLOUT) {
                        unsigned ack_is_64 = dcc_sessions.file_size[i] > UINT_MAX;
                        unsigned ack_shift = (1 - ack_is_64) * 32;
                        unsigned long long ack = htonll(dcc_sessions.bytes_read[i] << ack_shift);
                        write(dcc_sessions.sock_fds[i].fd, &ack, ack_is_64 ? 8 : 4);
                        if (dcc_sessions.bytes_read[i] == dcc_sessions.file_size[i]) {
                            shutdown(dcc_sessions.sock_fds[i].fd, SHUT_RDWR);
                            close(dcc_sessions.sock_fds[i].fd);
                            close(dcc_sessions.file_fds[i]);
                            dcc_sessions.sock_fds[i].fd = -1;
                        }
                    }
                }
            }
        } else {
            if (errno == EAGAIN) {
                continue;
            }
            perror("poll");
            return 1;
        }
    }
}
