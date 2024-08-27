/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2023 Michael Czigler
 */

#include "kirc.h"

static void free_history(void)
{
    if (!history) {
        return;
    }
    int j;
    for (j = 0; j < history_len; j++) {
        free(history[j]);
    }
    free(history);
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
    if (ioctl(1, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        return ws.ws_col;
    }
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
    if (cols <= start) {
        return cols;
    }
    char seq[32];
    snprintf(seq, sizeof(seq), "\x1b[%dD", cols - start);
    (void)write(ofd, seq, strnlen(seq, 32));
    return cols;
}

static int u8_character_size(char c)
{
    if (isu8 == 0){
         return 1;
    }
    int size = 0;
    while (c & (0x80 >> size++));
    return size ? size : 1;
}

static size_t u8_length(const char *s)
{
    size_t lenu8 = 0;
    while (*s != '\0') {
        lenu8 += u8_character_start(*s);
        s++;
    }
    return lenu8;
}

static size_t u8_previous(const char *s, size_t posb)
{
    if (posb == 0) {
        return 0;
    }
    do {
        posb--;
    } while ((posb > 0) && !u8_character_start(s[posb]));
    return posb;
}

static size_t u8_next(const char *s, size_t posb)
{
    if (s[posb] == '\0') {
        return posb;
    }
    do {
        posb++;
    } while ((s[posb] != '\0') && !u8_character_start(s[posb]));
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
    if (write(ofd, TESTCHARS, sizeof(TESTCHARS) - 1) != sizeof(TESTCHARS) - 1) {
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
    isu8 = 1;
    return 0;
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

static void refresh_line(state l)
{
    size_t plenu8 = l->plenu8 + 2;
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
    while (txtlenb < lenb && ch++ < l->cols) {
        txtlenb += u8_next(buf + txtlenb, 0);
    }
    ab.b = NULL;
    ab.len = 0;
    ab_append(&ab, "\r", sizeof("\r") - 1);
    ab_append(&ab, chan, l->plenb);
    ab_append(&ab, "> ", sizeof("> ") - 1);
    ab_append(&ab, buf, txtlenb);
    ab_append(&ab, "\x1b[0K", sizeof("\x1b[0K") - 1);
    if (posu8 + plenu8) {
        char seq[32];
        snprintf(seq, sizeof(seq), "\r\x1b[%dC", (int)(posu8 + plenu8));
        ab_append(&ab, seq, strnlen(seq, 32));
    } else {
        ab_append(&ab, "\r", sizeof("\r") - 1);
    }
    if (write(STDOUT_FILENO, ab.b, ab.len) == -1) {
    }
    free(ab.b);
}

static int edit_insert(state l, char *c)
{
    size_t clenb = strlen(c);
    if ((l->lenb + clenb) >= l->buflen) {
        return 0;
    }
    if (l->lenu8 != l->posu8) {
        buffer_position_move_end(l, clenb, 0);
        memmove(l->buf + l->posb, c, clenb);
        l->posu8++;
        l->lenu8++;
        l->posb += clenb;
        l->lenb += clenb;
        refresh_line(l);
        return 0;
    }
    strcpy(l->buf + l->posb, c);
    l->posu8++;
    l->lenu8++;
    l->posb += clenb;
    l->lenb += clenb;
    if ((l->plenu8 + l->lenu8) >= l->cols) {
        refresh_line(l);
        return 0;
    }
    if (write(STDOUT_FILENO, c, clenb) == -1) {
        return -1;
    }
    return 0;
}

static void edit_move_left(state l)
{
    if (l->posb == 0){
        return;
    }
    l->posb = u8_previous(l->buf, l->posb);
    l->posu8--;
    refresh_line(l);
}

static void edit_move_right(state l)
{
    if (l->posu8 == l->lenu8) {
        return;
    }
    l->posb = u8_next(l->buf, l->posb);
    l->posu8++;
    refresh_line(l);
}

static void edit_move_home(state l)
{
    if (l->posb == 0) {
        return;
    }
    l->posb = 0;
    l->posu8 = 0;
    refresh_line(l);
}

static void edit_move_end(state l)
{
    if (l->posu8 == l->lenu8) {
        return;
    }
    l->posb = l->lenb;
    l->posu8 = l->lenu8;
    refresh_line(l);
}

static void edit_delete(state l)
{
    if ((l->lenu8 == 0) || (l->posu8 >= l->lenu8)) {
        return;
    }
    size_t this_size = u8_next(l->buf, l->posb) - l->posb;
    buffer_position_move_end(l, 0, this_size);
    l->lenb -= this_size;
    l->lenu8--;
    refresh_line(l);
}

static void edit_backspace(state l)
{
    if ((l->posu8 == 0) || (l->lenu8 == 0)) {
        return;
    }
    size_t prev_size = l->posb - u8_previous(l->buf, l->posb);
    buffer_position_move_end(l, (ssize_t) - prev_size, 0);
    l->posb -= prev_size;
    l->lenb -= prev_size;
    l->posu8--;
    l->lenu8--;
    refresh_line(l);
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

static inline void edit_delete_whole_line(state l)
{
    l->buf[0] = '\0';
    l->posb = l->lenb = l->posu8 = l->lenu8 = 0;
    refresh_line(l);
}

static inline void edit_delete_line_to_end(state l)
{
    l->buf[l->posb] = '\0';
    l->lenb = l->posb;
    l->lenu8 = l->posu8;
    refresh_line(l);
}

static void edit_swap_character_w_previous(state l)
{
    if (l->posu8 == 0 || l->posu8 >= l->lenu8) {
        return;
    }
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

static void edit_history(state l, int dir)
{
    if (history_len <= 1) {
        return;
    }
    free(history[history_len - (1 + l->history_index)]);
    history[history_len - (1 + l->history_index)] = strdup(l->buf);
    l->history_index += (dir == 1) ? 1 : -1;
    if (l->history_index < 0) {
        l->history_index = 0;
        return;
    }
    if (l->history_index >= history_len) {
        l->history_index = history_len - 1;
        return;
    }
    strcpy(l->buf, history[history_len - (1 + l->history_index)]);
    l->buf[l->buflen - 1] = '\0';
    l->lenb = l->posb = strnlen(l->buf, MSG_MAX);
    l->lenu8 = l->posu8 = u8_length(l->buf);
    refresh_line(l);
}

static int history_add(const char *line)
{
    char *linecopy;
    if (history == NULL) {
        history = malloc(sizeof(char *) * HIS_MAX);
        if (history == NULL) {
            return 0;
        }
        memset(history, 0, (sizeof(char *) * HIS_MAX));
    }
    if (history_len && !strcmp(history[history_len - 1], line)) {
        return 0;
    }
    linecopy = strdup(line);
    if (!linecopy) {
        return 0;
    }
    if (history_len == HIS_MAX) {
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (HIS_MAX - 1));
        history_len--;
    }
    history[history_len] = linecopy;
    history_len++;
    return 1;
}

static inline void edit_enter(void)
{
    history_len--;
    free(history[history_len]);
}

static void edit_escape_sequence(state l, char seq[3])
{
    if (read(ttyinfd, seq, 1) == -1) {
        return;
    }
    if (read(ttyinfd, seq + 1, 1) == -1) {
        return;
    }
    if (seq[0] != '[') {        /* ESC [ sequences. */
        if (seq[0] != 'O') { /* ESC O sequences. */
            return;
        }
        switch (seq[1]) {
        case 'H':
            edit_move_home(l);
            return;              /* Home */
        case 'F':
            edit_move_end(l);
            return;              /* End */
        }
        return;
    }
    if ((seq[1] >= '0') && (seq[1] <= '9')) {
        /* Extended escape, read additional byte. */
        if (read(ttyinfd, seq + 2, 1) == -1) {
            return;
        }
        if ((seq[2] == '~') && (seq[1] == 3)) {
            edit_delete(l); /* Delete key. */
        }
        return;
    }
    switch (seq[1]) {
    case 'A':
        edit_history(l, 1);
        return;          /* Up */
    case 'B':
        edit_history(l, 0);
    return;          /* Down */
    case 'C':
        edit_move_right(l);
        return;          /* Right */
    case 'D':
        edit_move_left(l);
        return;          /* Left */
    case 'H':
        edit_move_home(l);
        return;          /* Home */
    case 'F':
        edit_move_end(l);
        return;          /* End */
    }
}
static int edit(state l)
{
    char c;
    if (read(ttyinfd, &c, 1) <= 0) {
        return 1;
    }
    char seq[3];
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
        return 0;              /* ctrl-h */
    case 2:
        edit_move_left(l);
        return 0;              /* ctrl-b */
    case 6:
        edit_move_right(l);
        return 0;              /* ctrl-f */
    case 1:
        edit_move_home(l);
        return 0;              /* Ctrl+a */
    case 5:
        edit_move_end(l);
        return 0;              /* ctrl+e */
    case 23:
        edit_delete_previous_word(l);
        return 0;              /* ctrl+w */
    case 21:
        edit_delete_whole_line(l);
        return 0;              /* Ctrl+u */
    case 11:
        edit_delete_line_to_end(l);
        return 0;              /* Ctrl+k */
    case 14:
        edit_history(l, 0);
        return 0;              /* Ctrl+n */
    case 16:
        edit_history(l, 1);
        return 0;              /* Ctrl+p */
    case 20:
        edit_swap_character_w_previous(l);
        return 0;              /* ctrl-t */
    case 27:
        edit_escape_sequence(l, seq);
        return 0;              /* escape sequence */
    case 4:                /* ctrl-d */
        if (l->lenu8 == 0) {
            history_len--;
            free(history[history_len]);
            return -1;
        }
        edit_delete(l);
        return 0;
    }
    if (!u8_character_start(c)) {
        return 0;
    }
    char aux[8];
    aux[0] = c;
    int size = u8_character_size(c);
    for (int i = 1; i < size; i++) {
        if(read(ttyinfd, aux + i, 1) == -1){
            break;
        }
        if ((aux[i] & 0xC0) != 0x80) {
            break;
        }
    }
    aux[size] = '\0';
    if (edit_insert(l, aux)) {
        return -1;
    }
    return 0;
}


static inline void state_reset(state l)
{
    l->plenb = strnlen(chan, MSG_MAX);
    l->plenu8 = u8_length(chan);
    l->oldposb = l->posb = l->oldposu8 = l->posu8 = l->lenb = l->lenu8 = 0;
    l->history_index = 0;
    l->buf[0] = '\0';
    l->buflen--;
    history_add("");
}

static char *ctime_now(char *buf)
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
    size_t spaceleft = p->maxcols - (p->nicklen + p->offset);
    for (tok = strtok(p->message, " "); tok != NULL; tok = strtok(NULL, " ")) {
        size_t wordwidth, spacewidth = 1;
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

static inline void param_print_nick(param p)
{
    printf("\x1b[35;1m%*s\x1b[0m ", p->nicklen - 4, p->nickname);
    printf("--> \x1b[35;1m%s\x1b[0m", p->message);
}

static void param_print_part(param p)
{
    printf("%*s<-- \x1b[34;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
    if (p->channel != NULL && strcmp(p->channel + 1, chan)) {
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
    }
}

static inline void param_print_quit(param p)
{
    printf("%*s<<< \x1b[34;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
}

static void param_print_join(param p)
{
    printf("%*s--> \x1b[32;1m%s\x1b[0m", p->nicklen - 3, "", p->nickname);
    if (p->channel != NULL && strcmp(p->channel + 1, chan)) {
        printf(" [\x1b[33m%s\x1b[0m] ", p->channel);
    }
}

static void print_error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("\r\x1b[31merror: ");
    vprintf(fmt, ap);
    printf("\x1b[0m\r\n");
    va_end(ap);
}

static short parse_dcc_send_message(const char *message, char *filename, unsigned int *ip_addr, char *ipv6_addr, unsigned short *port, size_t *file_size)
{
    /* TODO: Fix horrible hacks */

    if (sscanf(message, "SEND \"%" STR(FNM_MAX) "[^\"]\" %41s %hu %zu", filename, ipv6_addr, port, file_size) == 4) {
        if (ipv6_addr[15]) {
            return 1;
        }
    }
    if (sscanf(message, "SEND %" STR(FNM_MAX) "s %41s %hu %zu", filename, ipv6_addr, port, file_size) == 4) {
        if (ipv6_addr[15]) {
            return 1;
        }
    }
    if (sscanf(message, "SEND \"%" STR(FNM_MAX) "[^\"]\" %u %hu %zu", filename, ip_addr, port, file_size) == 4) {
        return 0;
    }
    if (sscanf(message, "SEND %" STR(FNM_MAX) "s %u %hu %zu", filename, ip_addr, port, file_size) == 4) {
        return 0;
    }
    print_error("unable to parse DCC message '%s'", message);
    return -1;
}

static short parse_dcc_accept_message(const char *message, char *filename, unsigned short *port, size_t *file_size)
{
    if (sscanf(message, "ACCEPT \"%" STR(FNM_MAX) "[^\"]\" %hu %zu", filename, port, file_size) == 3) {
        return 0;
    }
    if (sscanf(message, "ACCEPT %" STR(FNM_MAX) "s %hu %zu", filename, port, file_size) == 3) {
        return 0;
    }
    print_error("unable to parse DCC message '%s'", message);
    return 1;
}

static void open_socket(int slot, int file_fd)
{
    int sock_fd;
    if(!ipv6) {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in sockaddr = dcc_sessions.slots[slot].sin;
        if (connect(sock_fd, (const struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
            close(sock_fd);
            close(file_fd);
            perror("connect");
            return;
        }
    }
    else {
        sock_fd = socket(AF_INET6, SOCK_STREAM, 0);
        struct sockaddr_in6 sockaddr = dcc_sessions.slots[slot].sin6;
        if (connect(sock_fd, (const struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0) {
            close(sock_fd);
            close(file_fd);
            perror("connect");
            return;
        }
    }
    if (sock_fd < 0) {
        close(file_fd);
        perror("socket");
        return;
    }
    int flags = fcntl(sock_fd, F_GETFL, 0) | O_NONBLOCK;
    if (flags < 0 || fcntl(sock_fd, F_SETFL, flags) < 0) {
        close(sock_fd);
        close(file_fd);
        perror("fcntl");
        return;
    }
    dcc_sessions.sock_fds[slot].fd = sock_fd;
}

/* TODO: since we don't have config files how do we configure a download directory? */
static void handle_dcc(param p)
{
    const char *message = p->message + 5;
    char filename[FNM_MAX + 1];
    size_t file_size = 0;
    unsigned int ip_addr = 0;
    unsigned short port = 0;
    char ipv6_addr[INET6_ADDRSTRLEN];

    memset(ipv6_addr, 0, sizeof(ipv6_addr));

    int slot = -1;
    int file_fd;

    if (!strncmp(message, "SEND", 4)) {
        while(++slot < CON_MAX && dcc_sessions.slots[slot].file_fd >= 0);

        if (slot == CON_MAX) {
            raw("PRIVMSG %s :XDCC CANCEL\r\n", p->nickname);
            return;
        }

        /* TODO: the file size parameter is optional so this isn't strictly correct. */

        ipv6 = parse_dcc_send_message(message, filename, &ip_addr, ipv6_addr, &port, &file_size);
        if(ipv6 == -1) {
            return;
        }

        /* TODO: at this point we should make sure that the filename is actually a filename
           and not a file path. furthermore, it would be helpful to give the user
           the option to rename to the file.
        */

        int file_resume = 0;
        size_t bytes_read = 0;
        file_fd = open(filename, DCC_FLAGS);

        if (file_fd > 0) {
            struct stat statbuf = {0};
            if (fstat(file_fd, &statbuf)) {
                close(file_fd);
            }
            bytes_read = statbuf.st_size;
            file_resume = 1;
        }

        if (!file_resume) {
            file_fd = open(filename, DCC_FLAGS | O_CREAT, DCC_MODE);
        }

        if (file_fd < 0) {
            perror("open");
            exit(1);
        }

        if (file_size == bytes_read) {
            close(file_fd);
            return;
        }

        dcc_sessions.slots[slot] = (struct dcc_connection) {
            .bytes_read = bytes_read,
            .file_size = file_size,
            .file_fd = file_fd,
        };

        strcpy(dcc_sessions.slots[slot].filename, filename);
        if(!ipv6) {
            dcc_sessions.slots[slot].sin  = (struct sockaddr_in){
                .sin_family = AF_INET,
                .sin_addr = (struct in_addr){htonl(ip_addr)},
                .sin_port = htons(port),
            };
            goto check_resume;
        }

        struct in6_addr result;
        if (inet_pton(AF_INET6, ipv6_addr, &result) != 1){
            close(file_fd);
        }
        dcc_sessions.slots[slot].sin6 = (struct sockaddr_in6){
            .sin6_family = AF_INET6,
            .sin6_addr = result,
            .sin6_port = htons(port),
        };

check_resume:
        if (file_resume) {
            raw("PRIVMSG %s :\001DCC RESUME \"%s\" %hu %zu\001\r\n",
            p->nickname, filename, port, bytes_read);
            return;
        }
        open_socket(slot, file_fd);
        return;
    }

    if (!strncmp(message, "ACCEPT", 6)) {
        if(parse_dcc_accept_message(message, filename, &port, &file_size)) {
            return;
        }

        while(++slot < CON_MAX && strncmp(dcc_sessions.slots[slot].filename, filename, FNM_MAX));

        if (slot == CON_MAX) {
            return;
        }

        file_fd = dcc_sessions.slots[slot].file_fd;
        open_socket(slot, file_fd);
        return;
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
        return;
    }
    if (!strncmp(message, "TIME", 4)) {
        char buf[26];
        if (!ctime_now(buf)) {
            raw("NOTICE %s :\001TIME %s\001\r\n", p->nickname, buf);
        }
        return;
    }
    if (!strncmp(message, "CLIENTINFO", 10)) {
        raw("NOTICE %s :\001CLIENTINFO " CTCP_CMDS "\001\r\n", p->nickname);
        return;
    }if (!strncmp(message, "PING", 4)) {
        raw("NOTICE %s :\001%s\r\n", p->nickname, message);
        return;
    }
    if (!strncmp(message, "DCC", 3)) {
        handle_dcc(p);
        return;
    }
}

static void param_print_private(param p)
{
    time_t rawtime;
    struct tm *timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    char timestamp[9];
    if (!small_screen) {
        if (strnlen(p->nickname, p->nicklen) > (size_t)p->nicklen - 8) {
            *(p->nickname + p->nicklen - 8) = '\0';
        }
        strcpy(timestamp, "[");
        char buf[5];
        if (timeinfo->tm_hour < 10) {
            strcat(timestamp, "0");
        }
        snprintf(buf, sizeof(buf), "%d:", timeinfo->tm_hour);
        strcat(timestamp, buf);
        if (timeinfo->tm_min < 10) {
            strcat(timestamp, "0");
        }
        snprintf(buf, sizeof(buf), "%d] ", timeinfo->tm_min);
        strcat(timestamp, buf);
        printf("%s", timestamp);
    }
    int s = 0;
    if (strnlen(p->nickname, MSG_MAX) <= (size_t)p->nicklen + strnlen(timestamp, sizeof(timestamp))) {
        s = p->nicklen - strnlen(p->nickname, MSG_MAX) - strnlen(timestamp, sizeof(timestamp));
    }
    if (p->channel != NULL && (strcmp(p->channel, nick) == 0)) {
        handle_ctcp(p);
        printf("%*s\x1b[33;1m%-.*s [PRIVMSG]\x1b[36m ", s, "", p->nicklen, p->nickname);
        p->offset += sizeof(" [PRIVMSG]");
    } else if (p->channel != NULL && strcmp(p->channel + 1, chan)) {
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
    param_t p = {
        .prefix = strtok(string, " ") + 1,
        .suffix = strtok(NULL, ":"),
        .message = strtok(NULL, "\r"),
        .nickname = strtok(p.prefix, "!"),
        .command = strtok(p.suffix, "#& "),
        .channel = strtok(NULL, " \r"),
        .params = strtok(NULL, ":\r"),
        .maxcols = get_columns(ttyinfd, STDOUT_FILENO),
        .offset = 0
    };
    if (WRAP_LEN > p.maxcols / 3) {
        small_screen = 1;
        p.nicklen = p.maxcols / 3;
    }
    else {
        small_screen = 0;
        p.nicklen = WRAP_LEN;
    }
    if (!strncmp(p.command, "001", 3) && *chan != '\0') {
        char *tok;
        for (tok = strtok(chan, ",|"); tok != NULL; tok = strtok(NULL, ",|")) {
            strcpy(chan, tok);
            raw("JOIN #%s\r\n", tok);
        }
        return;
    }
    if (!strncmp(p.command, "QUIT", 4)) {
        param_print_quit(&p);
        printf("\x1b[0m\r\n");
        return;
    }if (!strncmp(p.command, "PART", 4)) {
        param_print_part(&p);
        printf("\x1b[0m\r\n");
        return;
    }if (!strncmp(p.command, "JOIN", 4)) {
        param_print_join(&p);
        printf("\x1b[0m\r\n");
        return;
    }if (!strncmp(p.command, "NICK", 4)) {
        param_print_nick(&p);
        printf("\x1b[0m\r\n");
        return;
    }if (!strncmp(p.command, "PRIVMSG", 7)) {
        param_print_private(&p);
        message_wrap(&p);
        printf("\x1b[0m\r\n");
        return;
    }
    param_print_channel(&p);
    message_wrap(&p);
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
            }
            perror("read");
            return -2;
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

static void join_command(state l)
{
    if (!strchr(l->buf, '#')){
        printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
        printf("\x1b[35mIllegal channel!\x1b[0m\r\n");
        return;
    }
    *stpncpy(chan, strchr(l->buf, '#') + 1, MSG_MAX - 1) = '\0';
    raw("JOIN #%s\r\n", chan);
    printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
    printf("\x1b[35mJoined #%s!\x1b[0m\r\n", chan);
    l->nick_privmsg = 0;
}

static void part_command(state l)
{
    char *tok;
    tok = strchr(l->buf, '#');
    if (tok){
        raw("PART %s\r\n", tok);
        printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
        printf("\x1b[35mLeft %s!\r\n", tok);
        printf("\x1b[35mYou need to use /join or /# to speak in a channel!\x1b[0m\r\n");
        strcpy(chan, "");
        return;
    }
    tok = l->buf + 5;
    while (*tok == ' ') {
       tok++;
    }
    if (*tok == '#' || *tok == '\0') {
        raw("PART #%s\r\n", chan);
        printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
        printf("\x1b[35mLeft #%s!\r\n", chan);
        printf("\x1b[35mYou need to use /join or /# to speak in a channel!\x1b[0m\r\n");
        strcpy(chan, "");
        return;
    }
    printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
    printf("\x1b[35mIllegal channel!\x1b[0m\r\n");
}

static void msg_command(state l)
{
    char *tok;
    strtok_r(l->buf + 4, " ", &tok);
    int offset = 0;
    while (*(l->buf + 4 + offset) == ' ') {
        offset ++;
    }
    raw("PRIVMSG %s :%s\r\n", l->buf + 4 + offset, tok);
    if (strncmp(l->buf + 4 + offset, "NickServ", 8)) {
        printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", l->buf + 4 + offset, tok);
    }
}

static void action_command(state l)
{
    int offset = 0;
    while (*(l->buf + 7 + offset) == ' ') {
        offset ++;
    }

    raw("PRIVMSG #%s :\001ACTION %s\001\r\n", chan, l->buf + 7 + offset);
    printf("\x1b[35mprivmsg #%s :ACTION %s\x1b[0m\r\n", chan, l->buf + 7 + offset);
}

static void set_privmsg_command(state l)
{
    int offset = 0;
    while (*(l->buf + 11 + offset) == ' ') {
        offset ++;
    }

    strcpy(chan, l->buf + 11 + offset);

    printf("\x1b[35mNew privmsg target: %s\x1b[0m\r\n", l->buf + 11 + offset);
    l->nick_privmsg = 1;
}


static void nick_command(state l)
{
    char *tok;
    raw("%s\r\n", l->buf + 1);
    printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
    tok = l->buf + 5;
    while (*tok == ' ') {
        tok ++;
    }
    strcpy(nick, tok);
}

static void handle_user_input(state l)
{
    if (*l->buf == '\0') {
        return;
    }
    char *tok;
    size_t msg_len = strnlen(l->buf, MSG_MAX);
    if (msg_len > 0 && l->buf[msg_len - 1] == '\n') {
        l->buf[msg_len - 1] = '\0';
    }
    printf("\r\x1b[0K");
    switch (l->buf[0]) {
    case '/':           /* send system command */
        if (!strncmp(l->buf + 1, "JOIN", 4) || !strncmp(l->buf + 1, "join", 4)) {
            join_command(l);
            return;
        }
        if (!strncmp(l->buf + 1, "PART", 4) || !strncmp(l->buf + 1, "part", 4)) {
            part_command(l);
            return;
        }
        if (l->buf[1] == '/') {
            raw("PRIVMSG #%s :%s\r\n", chan, l->buf + 2);
            printf("\x1b[35mprivmsg #%s :%s\x1b[0m\r\n", chan, l->buf + 2);
            return;
        }
        if (!strncmp(l->buf + 1, "MSG", 3) || !strncmp(l->buf + 1, "msg", 3)) {
            msg_command(l);
            return;
        }
        if (!strncmp(l->buf + 1, "NICK", 4) || !strncmp(l->buf + 1, "nick", 4)) {
            nick_command(l);
            return;
        }
        if (!strncmp(l->buf + 1, "ACTION", 6) || !strncmp(l->buf + 1, "action", 6)) {
            action_command(l);
            return;
        }
        if (!strncmp(l->buf + 1, "SETPRIVMSG", 10) || !strncmp(l->buf + 1, "setprivmsg", 10)) {
            set_privmsg_command(l);
            return;
        }

        if (l->buf[1] == '#') {
            strcpy(chan, l->buf + 2);
            l->nick_privmsg = 0;
            printf("\x1b[35mnew channel: #%s\x1b[0m\r\n", chan);
            return;
        }
        raw("%s\r\n", l->buf + 1);
        printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
        return;
    case '@':           /* send private message to target channel or user */
        strtok_r(l->buf, " ", &tok);
        if (l->buf[1] != '@') {
            raw("PRIVMSG %s :%s\r\n", l->buf + 1, tok);
            printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", l->buf + 1, tok);
            return;
        }
        raw("PRIVMSG %s :\001ACTION %s\001\r\n", l->buf + 2, tok);
        printf("\x1b[35mprivmsg %s :ACTION %s\x1b[0m\r\n", l->buf + 2, tok);
        return;
    default:           /*  send private message to default channel */
        if(l->nick_privmsg == 0) {
            raw("PRIVMSG #%s :%s\r\n", chan, l->buf);
            printf("\x1b[35mprivmsg #%s :%s\x1b[0m\r\n", chan, l->buf);
        }
        else {
            raw("PRIVMSG %s :%s\r\n", chan, l->buf);
            printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", chan, l->buf);
        }
        return;
    }
}

static inline void usage(void)
{
    fputs("kirc [-s host] [-p port] [-c channel] [-n nick] [-r realname] \
[-u username] [-k password] [-a token] [-o path] [-e] [-x] [-v] [-V]\n", stderr);
    exit(2);
}

static inline void version(void)
{
    fputs("kirc-" VERSION " Copyright © 2022 Michael Czigler, MIT License\n",
          stdout);
    exit(0);
}

static inline void slot_clear(size_t i) {
    memset(dcc_sessions.slots[i].filename, 0, FNM_MAX);
    dcc_sessions.sock_fds[i] = (struct pollfd){.fd = -1, .events = POLLIN | POLLOUT};
    dcc_sessions.slots[i] = (struct dcc_connection){.file_fd = -1};
}

/* TODO: implicitly assumes that the size reported was the correct size */
/* TODO: should we just close the file and keep on running if we get
   an error we can recover from? */
static void slot_process(state l, char *buf, size_t buf_len, size_t i) {
    const char *err_str;
    int sock_fd = dcc_sessions.sock_fds[i].fd;
    int file_fd = dcc_sessions.slots[i].file_fd;

    if (~dcc_sessions.sock_fds[i].revents & POLLIN) {
        return;
    }

    int n = read(sock_fd, buf, buf_len);
    if (n == -1) {
        err_str = "read";
        goto handle_err;
    }

    dcc_sessions.slots[i].bytes_read += n;
    if (write(file_fd, buf, n) < 0) {
        err_str = "write";
        goto handle_err;
    }

    if (!(dcc_sessions.sock_fds[i].revents & POLLOUT)) {
        refresh_line(l);
        return;
    }
    size_t file_size = dcc_sessions.slots[i].file_size;
    size_t bytes_read = dcc_sessions.slots[i].bytes_read;
    unsigned ack_is_64 = file_size > UINT_MAX;
    unsigned ack_shift = (1 - ack_is_64) * 32;
    unsigned long long ack = htonll(bytes_read << ack_shift);
    if (write(sock_fd, &ack, ack_is_64 ? 8 : 4) < 0) {
        err_str = "write";
        goto handle_err;
    }
    if (bytes_read == file_size) {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        close(file_fd);
        slot_clear(i);
    }
    refresh_line(l);
    return;
handle_err:
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
    }
    if (errno == ECONNRESET) {
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
        close(file_fd);
        slot_clear(i);
        return;
    } else {
        perror(err_str);
        return;
    }
}

int main(int argc, char **argv)
{
    char buf[BUFSIZ];
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
            strcpy(chan, optarg);
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
        slot_clear(i);
    }
    dcc_sessions.sock_fds[CON_MAX] = (struct pollfd){.fd = ttyinfd,.events = POLLIN};
    dcc_sessions.sock_fds[CON_MAX + 1] = (struct pollfd){.fd = conn,.events = POLLIN};
    state_t l;
    memset(&l, 0, sizeof(l));
    l.buflen = MSG_MAX;
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
                if (rc == -2) {
                    return 1;
                }
                if (rc != 0) {
                    return 0;
                }
                refresh_line(&l);
            }

            for (int i = 0; i < CON_MAX; i++) {
                slot_process(&l, buf, sizeof(buf), i);
            }
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            perror("poll");
            return 1;
        }
    }
}
