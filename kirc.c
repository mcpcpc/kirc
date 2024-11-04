/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2023 Michael Czigler
 */

#include "kirc.h"

static void disable_raw_mode(void)
{
    tcsetattr(ttyinfd, TCSAFLUSH, &orig);
}

static int enable_raw_mode(int fd)
{
    if (!isatty(ttyinfd)) {
        goto fatal;
    }
    if (tcgetattr(fd, &orig) == -1) {
        goto fatal;
    }
    atexit(disable_raw_mode);
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
    write(ofd, seq, strnlen(seq, sizeof(seq)));
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
    if (len + ab->len > (int)ABUF_LEN) {
        return;
    }
    memcpy(ab->b + ab->len, s, len);
    ab->len += len;
}

static void refresh_line(state l)
{
    size_t plenu8 = l->plenu8 + 2;
    char *buf;
    for (buf = l->buf; *buf; buf++) {
        switch (*buf) {
        case '\t':
        case '\031':
            *buf = ' ';
            break;
        }
    }
    buf = l->buf;
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
}

static int edit_insert(state l, char *c)
{
    size_t clenb = strlen(c);
    if ((l->lenb + clenb) >= MSG_MAX - 1) {
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
    int idx = (((history_len - 1 - l->history_index) >= 0) ? history_len - 1 - l->history_index
                                                           : (history_wrap ? HIS_MAX + history_len - 1 - l->history_index : -1));
    if (idx < 0) {
        return;
    }
    strcpy(history[idx], l->buf);
    l->history_index += dir;
    if (l->history_index < 0) {
        l->history_index = 0;
        return;
    }
    if (l->history_index >= history_len) {
        if (!history_wrap) {
            l->history_index = history_len - 1;
            return;
        }
        if (l->history_index >= HIS_MAX) {
            l->history_index = HIS_MAX - 1;
            return;
        }
    }
    idx = (((history_len - 1 - l->history_index) >= 0) ? history_len - 1 - l->history_index
                                                       : (history_wrap ? HIS_MAX + history_len - 1 - l->history_index : -1));
    if (idx < 0) {
        return;
    }
    l->lenb = l->posb = strnlen(history[idx], MSG_MAX);
    l->lenu8 = l->posu8 = u8_length(history[idx]);
    memcpy(l->buf, history[idx], l->lenb + 1);
    refresh_line(l);
}

static int history_add(const char *line)
{
    int idx = history_len ? history_len : (history_wrap ? HIS_MAX : 0);
    if (idx && !strcmp(history[idx - 1], line)) {
        return 0;
    }
    if (history_len == HIS_MAX) {
        history_len = 0;
        history_wrap = 1;
    }
    strcpy(history[history_len], line);
    history_len++;
    return 1;
}

static inline void edit_enter(void)
{
    history_len--;
    if (history_len == -1 && history_wrap) {
        history_len = HIS_MAX - 1;
        history_wrap = 0;
    }
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
        edit_history(l, -1);
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
        edit_history(l, -1);
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
            if (history_len == -1 && history_wrap) {
                history_len = HIS_MAX - 1;
                history_wrap = 0;
            }
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
    static char not_first_time = 0;
    if (not_first_time) {
        l->plenb = strnlen(chan, MSG_MAX);
        l->plenu8 = u8_length(chan);
    }
    else {
        not_first_time = 1;
        char *tok = chan;
        char *ptr;
        for(ptr = chan; *ptr; ptr++) {
            if ((*ptr == ',') || (*ptr == '|')) {
                tok = ptr + 1;
            }
        }
        l->plenb = ptr - tok;
        l->plenu8 = u8_length(tok);
    }
    l->oldposb = l->posb = l->oldposu8 = l->posu8 = l->lenb = l->lenu8 = 0;
    l->history_index = 0;
    l->buf[0] = '\0';
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

static void print_error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    printf("\r\x1b[31merror: ");
    vprintf(fmt, ap);
    printf("\x1b[0m\r\n");
    va_end(ap);
}

static void log_append(char *str, char *path)
{
    FILE *out;
    char buf[26];
    static char append_failed;
    if ((out = fopen(path, "a")) == NULL) {
        if (!append_failed) {
            append_failed = 1;
            print_error("fopen failed, logging disabled");
            perror("fopen");
        }
        return;
    }
    if (append_failed) {
        append_failed = 0;
        print_error("logging back on");
    }
    ctime_now(buf);
    char _o_str[MSG_MAX + 1]; /* str is at most this big */
    char *o_str = _o_str;
    while(*str != '\0') {
        if ((*str >= 32 && *str < 127) || *str == '\n') {
            *o_str = *str;
            o_str++;
        }
        str++;
    }
    *o_str = '\0';
    fprintf(out, "%s:%s", buf, _o_str);
    fclose(out);
}

static void raw(char *fmt, ...)
{
    va_list ap;
    char cmd_str[MSG_MAX + 1];
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
}

static int connection_initialize(void)
{
    int gai_status;
    struct addrinfo *res, hints = {
        .ai_family = hint_family,
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

static sa_family_t parse_dcc_send_message(const char *message, char *filename, unsigned int *ip_addr, char *ipv6_addr, unsigned short *port, unsigned long long *file_size)
{
    if (sscanf(message, "SEND \"%" STR(FNM_MAX) "[^\"]\" %" STR(INET6_ADDRSTRLEN) "s %hu %llu", filename, ipv6_addr, port, file_size) == 4) {
        if (strchr(ipv6_addr, ':')) {
            return AF_INET6;
        }
    }
    if (sscanf(message, "SEND %" STR(FNM_MAX) "s %" STR(INET6_ADDRSTRLEN) "s %hu %llu", filename, ipv6_addr, port, file_size) == 4) {
        if (strchr(ipv6_addr, ':')) {
            return AF_INET6;
        }
    }
    if (sscanf(message, "SEND \"%" STR(FNM_MAX) "[^\"]\" %u %hu %llu", filename, ip_addr, port, file_size) == 4) {
        return AF_INET;
    }
    if (sscanf(message, "SEND %" STR(FNM_MAX) "s %u %hu %llu", filename, ip_addr, port, file_size) == 4) {
        return AF_INET;
    }
    /* filesize not given */
    if (sscanf(message, "SEND \"%" STR(FNM_MAX) "[^\"]\" %" STR(INET6_ADDRSTRLEN) "s %hu", filename, ipv6_addr, port) == 3) {
        if (strchr(ipv6_addr, ':')) {
            *file_size = ULLONG_MAX;
            return AF_INET6;
        }
    }
    if (sscanf(message, "SEND %" STR(FNM_MAX) "s %" STR(INET6_ADDRSTRLEN) "s %hu", filename, ipv6_addr, port) == 3) {
        if (strchr(ipv6_addr, ':')) {
            *file_size = ULLONG_MAX;
            return AF_INET6;
        }
    }
    if (sscanf(message, "SEND \"%" STR(FNM_MAX) "[^\"]\" %u %hu", filename, ip_addr, port) == 3) {
        *file_size = ULLONG_MAX;
        return AF_INET;
    }
    if (sscanf(message, "SEND %" STR(FNM_MAX) "s %u %hu", filename, ip_addr, port) == 3) {
        *file_size = ULLONG_MAX;
        return AF_INET;
    }
    print_error("unable to parse DCC message '%s'", message);
    return AF_UNSPEC;
}

static char parse_dcc_accept_message(const char *message, char *filename, unsigned short *port, unsigned long long *file_size)
{
    if (sscanf(message, "ACCEPT \"%" STR(FNM_MAX) "[^\"]\" %hu %llu", filename, port, file_size) == 3) {
        return 0;
    }
    if (sscanf(message, "ACCEPT %" STR(FNM_MAX) "s %hu %llu", filename, port, file_size) == 3) {
        return 0;
    }
    /* filesize not given */
    if (sscanf(message, "ACCEPT \"%" STR(FNM_MAX) "[^\"]\" %hu", filename, port) == 2) {
        *file_size = ULLONG_MAX;
        return 0;
    }
    if (sscanf(message, "ACCEPT %" STR(FNM_MAX) "s %hu", filename, port) == 2) {
        *file_size = ULLONG_MAX;
        return 0;
    }
    print_error("unable to parse DCC message '%s'", message);
    return 1;
}

static void open_socket(int slot, int file_fd)
{
    int sock_fd = socket(dcc_sessions.slots[slot].sin46.sin_family, SOCK_STREAM, 0);
    if (connect(sock_fd, (const struct sockaddr *)&dcc_sessions.slots[slot].sin46,
                         (dcc_sessions.slots[slot].sin46.sin_family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                                                  sizeof(struct sockaddr_in6)) < 0) {
        close(sock_fd);
        close(file_fd);
        perror("connect");
        return;
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

static inline void slot_clear(size_t i) {
    memset(dcc_sessions.slots[i].filename, 0, FNM_MAX);
    dcc_sessions.sock_fds[i] = (struct pollfd){.fd = -1, .events = POLLIN | POLLOUT};
    dcc_sessions.slots[i] = (struct dcc_connection){.file_fd = -1};
}

static void handle_dcc(param p)
{
    if (!dcc) {
        return;
    }

    const char *message = p->message + 5;
    char _filename[FNM_MAX + 1];
    char *filename = _filename;
    unsigned long long file_size = 0;
    unsigned int ip_addr = 0;
    unsigned short port = 0;
    char ipv6_addr[INET6_ADDRSTRLEN];

    memset(ipv6_addr, 0, sizeof(ipv6_addr));

    int slot = -1;
    int file_fd;

    if (!memcmp(message, "SEND", sizeof("SEND") - 1)) {
        while(++slot < CON_MAX && dcc_sessions.slots[slot].file_fd >= 0);

        if (slot == CON_MAX) {
            raw("PRIVMSG %s :XDCC CANCEL\r\n", p->nickname);
            print_error("DCC slots full");
            return;
        }

        slot_clear(slot);

        sa_family_t sin_family = parse_dcc_send_message(message, filename, &ip_addr, ipv6_addr, &port, &file_size);
        if (sin_family == AF_UNSPEC) {
            return;
        }

        if (port == 0) {
            /* don't cancel the transfer, just move on */
            print_error("Reverse DCC not implemented");
            return;
        }

        dcc_sessions.slots[slot].write = 0;

        for(int i = 0; _filename[i]; i++) {
            if (_filename[i] == '/') {
                filename = _filename + i + 1;
            }
        }

        char filepath[DIR_MAX + FNM_MAX  + 2];

        if (dcc_dir) {
            char *ptr = stpncpy(filepath, dcc_dir, DIR_MAX);
            *ptr = '\0';
            if (ptr != filepath && *(ptr - 1) != '/') {
                *ptr = '/';
                ptr++;
                *ptr = '\0';
            }

            strcpy(ptr, filename);

            filename = filepath;
        }

        int file_resume = 0;
        unsigned long long bytes_read = 0;
        file_fd = open(filename, DCC_FLAGS);

        if (file_fd >= 0) {
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
            print_error("couldn't open file for writing");
            perror("open");
            return;
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
        if(sin_family == AF_INET) {
            dcc_sessions.slots[slot].sin46.sin  = (struct sockaddr_in){
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
        dcc_sessions.slots[slot].sin46.sin6 = (struct sockaddr_in6){
            .sin6_family = AF_INET6,
            .sin6_addr = result,
            .sin6_port = htons(port),
        };

check_resume:
        if (file_resume) {
            raw("PRIVMSG %s :\001DCC RESUME \"%s\" %hu %llu\001\r\n",
            p->nickname, filename, port, bytes_read);
            return;
        }
        open_socket(slot, file_fd);
        return;
    }

    if (!memcmp(message, "ACCEPT", sizeof("ACCEPT") - 1)) {
        if(parse_dcc_accept_message(message, filename, &port, &file_size)) {
            return;
        }

        while(++slot < CON_MAX && strncmp(dcc_sessions.slots[slot].filename, filename, FNM_MAX));

        if (slot == CON_MAX) {
            return;
        }

        dcc_sessions.slots[slot].write = 0;

        file_fd = dcc_sessions.slots[slot].file_fd;
        open_socket(slot, file_fd);
        return;
    }
}

static void handle_ctcp(param p)
{
    if (p->message[0] != '\001' && memcmp(p->message, "ACTION", sizeof("ACTION") - 1)) {
        return;
    }
    const char *message = p->message + 1;
    if (!memcmp(message, "VERSION", sizeof("VERSION") - 1)) {
        raw("NOTICE %s :\001VERSION kirc " VERSION "\001\r\n", p->nickname);
        return;
    }
    if (!memcmp(message, "TIME", sizeof("TIME") - 1)) {
        char buf[26];
        if (!ctime_now(buf)) {
            raw("NOTICE %s :\001TIME %s\001\r\n", p->nickname, buf);
        }
        return;
    }
    if (!memcmp(message, "CLIENTINFO", sizeof("CLIENTINFO") - 1)) {
        raw("NOTICE %s :\001CLIENTINFO " CTCP_CMDS "\001\r\n", p->nickname);
        return;
    }if (!memcmp(message, "PING", sizeof("PING") - 1)) {
        raw("NOTICE %s :\001%s\r\n", p->nickname, message);
        return;
    }
    if (!memcmp(message, "DCC", sizeof("DCC") - 1)) {
        handle_dcc(p);
        return;
    }
}

static void filter_colors(char *string)
{
    if (!filter || !string) {
        return;
    }
    char _str1[MSG_MAX + 1]; /* string is at most this large */
    char *str1 = _str1;
    char *str = string;
    while(*str) {
        if (*str != '\003' && *str != '\004') {
            *str1 = *str;
            str1++;
            str++;
            continue;
        }
        /* color codes are ^Cxx,yy */
        /* xx and yy are numbers, either 1 or 2 digits */
        /* the ,yy part is optional */
        str++;
        char *p = str;
        while (*p && *p != ',' && *p >= '0' && *p <= '9' && p < str + 2) {
            p++;
        }
        str = p;
        if (*str != ',') {
            continue;
        }
        str++;
        p = str;
        while (*p && *p >= '0' && *p <= '9' && p < str + 2) {
            p++;
        }
        str = p;
    }
    *str1 = '\0';
    memcpy(string, _str1, str1 - _str1 + 1);
    return;
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
    if (!memcmp(p->message, "\x01" "ACTION", sizeof("\x01" "ACTION") - 1)) {
        p->message += sizeof("ACTION");
        p->offset += sizeof("[ACTION] ");
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
    if (!memcmp(string, "PING", sizeof("PING") - 1)) {
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
    if (*chan != '\0' && !memcmp(p.command, "001", sizeof("001") - 1)) {
        static char not_first_time = 0;
        if (not_first_time) {
            raw("JOIN #%s\r\n", chan);
            return;
        }
        char *tok = chan;
        char *ptr;
        for(ptr = chan; *ptr; ptr++) {
            if ((*ptr == ',') || (*ptr == '|')) {
                *ptr = '\0';
                if (ptr != tok) { /* the first encountered ',' or '|' */
                    raw("JOIN #%s\r\n", tok);
                }
                tok = ptr + 1;
            }
        }
        if (ptr == tok) { /* last char passed to -c was ',' or '|' */
            *chan = '\0';
            return;
        }
        raw("JOIN #%s\r\n", tok);
        memmove(chan, tok, ptr - tok + 1);
        return;
    }
    if (!memcmp(p.command, "QUIT", sizeof("QUIT") - 1)) {
        param_print_quit(&p);
        printf("\x1b[0m\r\n");
        return;
    }if (!memcmp(p.command, "PART", sizeof("PART") - 1)) {
        param_print_part(&p);
        printf("\x1b[0m\r\n");
        return;
    }if (!memcmp(p.command, "JOIN", sizeof("JOIN") - 1)) {
        param_print_join(&p);
        printf("\x1b[0m\r\n");
        return;
    }if (!memcmp(p.command, "NICK", sizeof("NICK") - 1)) {
        param_print_nick(&p);
        printf("\x1b[0m\r\n");
        return;
    }if ((!memcmp(p.command, "PRIVMSG", sizeof("PRIVMSG") - 1)) || (!memcmp(p.command, "NOTICE", sizeof("NOTICE") - 1))) {
        filter_colors(p.message); /* this can be slow if -f is passed to kirc */
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

static inline void join_command(state l)
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

static inline void part_command(state l)
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
    tok = l->buf + sizeof("part");
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

static inline void msg_command(state l)
{
    char *tok;
    strtok_r(l->buf + sizeof("msg"), " ", &tok);
    int offset = 0;
    while (*(l->buf + sizeof("msg") + offset) == ' ') {
        offset ++;
    }
    raw("PRIVMSG %s :%s\r\n", l->buf + sizeof("msg") + offset, tok);
    if (memcmp(l->buf + sizeof("msg") + offset, "NickServ", sizeof("NickServ") - 1)) {
        printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", l->buf + sizeof("msg") + offset, tok);
    }
}

static inline void action_command(state l)
{
    int offset = 0;
    while (*(l->buf + sizeof("action") + offset) == ' ') {
        offset ++;
    }

    raw("PRIVMSG #%s :\001ACTION %s\001\r\n", chan, l->buf + sizeof("action") + offset);
    printf("\x1b[35mprivmsg #%s :ACTION %s\x1b[0m\r\n", chan, l->buf + sizeof("action") + offset);
}

static inline void query_command(state l)
{
    int offset = 0;
    while (*(l->buf + sizeof("query") + offset) == ' ') {
        offset ++;
    }

    strcpy(chan, l->buf + sizeof("query") + offset);

    printf("\x1b[35mNew privmsg target: %s\x1b[0m\r\n", l->buf + sizeof("query") + offset);
    l->nick_privmsg = 1;
}


static inline void nick_command(state l)
{
    char *tok;
    raw("%s\r\n", l->buf + 1);
    printf("\x1b[35m%s\x1b[0m\r\n", l->buf);
    tok = l->buf + sizeof("nick");
    while (*tok == ' ') {
        tok++;
    }
    strcpy(nick, tok);
}

static void dcc_command(state l)
{
    printf("\x1b[35m%s\x1b[0m\r\n", l->buf);

    int slot = 0;
    while(++slot < CON_MAX && dcc_sessions.slots[slot].file_fd >= 0);

    if (slot == CON_MAX) {
        print_error("DCC slots full");
        return;
    }

    slot_clear(slot);

    char *tok = l->buf + sizeof("dcc");
    while (*tok == ' ') {
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        return;
    }

    char target[CHA_MAX + 1]; /* limit nicks to CHA_MAX */
    char *ptarget = target;

    int num = 0;

    while (*tok && *tok != ' ' && ++num < CHA_MAX) {
        *ptarget++ = *tok++;
    }

    *ptarget = '\0';

    if (*tok == '\0' || *tok != ' ') {
        print_error("malformed DCC command");
        return;
    }

    while (*tok == ' ') {
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        return;
    }

    char *filepath = tok;
    char *filename = tok;
    while (*tok && *tok != ' ') {
        if (*tok == '/') {
            filename = tok + 1;
        }
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        return;
    }

    *tok = '\0'; /* *tok was ' ' */


    dcc_sessions.slots[slot].filename[0] = '"';
    char *stp_cpy = stpncpy(dcc_sessions.slots[slot].filename + 1, filename, FNM_MAX - 2);
    *stp_cpy = '"';
    *(stp_cpy + 1) = '\0';
    int file_fd = open(filepath, O_RDONLY);

    *tok = ' '; /* put back *tok */

    if (file_fd < 0) {
        perror("open");
        return;
    }

    while (*tok == ' ') {
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        goto close_fd;
    }

    dcc_sessions.slots[slot].sin46.sin_family = AF_INET;

    char *ip_ptr = tok;

    while (*tok && *tok != ' ') {
        if (*tok == ':') {
            dcc_sessions.slots[slot].sin46.sin_family = AF_INET6;
        }
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        goto close_fd;
    }

    *tok = '\0'; /* *tok was ' ' */

    if (dcc_sessions.slots[slot].sin46.sin_family == AF_INET) {
        if (inet_pton(AF_INET, ip_ptr, &dcc_sessions.slots[slot].sin46.sin.sin_addr) != 1) {
            print_error("bad internal address");
            goto close_fd;
        }
    }
    else {
        if (inet_pton(AF_INET6, ip_ptr, &dcc_sessions.slots[slot].sin46.sin6.sin6_addr) != 1) {
            print_error("bad internal address");
            goto close_fd;
        }
    }

    *tok = ' '; /* put back *tok */

    while (*tok == ' ') {
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        goto close_fd;
    }

    ip_ptr = tok;

    struct sockaddr_in result;

    result.sin_family = AF_INET;

    while (*tok && *tok != ' ') {
        if (*tok == ':') {
            result.sin_family = AF_INET6;
        }
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        goto close_fd;
    }

    *tok = '\0'; /* *tok was ' ' */

    char ip_addr_string[INET6_ADDRSTRLEN];
    memset(ip_addr_string, 0, sizeof(ip_addr_string));

    if (result.sin_family == AF_INET) {
        if (inet_pton(AF_INET, ip_ptr, &result.sin_addr) != 1) {
            print_error("bad external address");
            goto close_fd;
        }
        int ind = 0;
        unsigned int ipv4_addr = htonl((unsigned int)result.sin_addr.s_addr);
        while (ipv4_addr) {
            ip_addr_string[ind] = (char)(ipv4_addr % 10 + '0');
            ipv4_addr /= 10;
            ind++;
        }
        char *begin = ip_addr_string;
        char *end = strchr(ip_addr_string, '\0') - 1;
        while (begin < end) {
            char tmp = *begin;
            *begin = *end;
            *end = tmp;
            begin++;
            end--;
        }
    }
    else {
        strcpy(ip_addr_string, ip_ptr);
    }

    *tok = ' '; /* put back *tok */

    while (*tok == ' ') {
        tok++;
    }

    if (*tok == '\0') {
        print_error("malformed DCC command");
        goto close_fd;
    }

    char *s_chr = strchr(tok, ' ');
    if (s_chr) {
        *s_chr = '\0';
    }

    if (dcc_sessions.slots[slot].sin46.sin_family == AF_INET) {
        for(char *ptr = tok; *ptr; ptr++) {
            dcc_sessions.slots[slot].sin46.sin.sin_port *= 10;
            dcc_sessions.slots[slot].sin46.sin.sin_port += *ptr - '0';
        }
        dcc_sessions.slots[slot].sin46.sin.sin_port = htons(dcc_sessions.slots[slot].sin46.sin.sin_port);
    }
    else {
        for(char *ptr = tok; *ptr; ptr++) {
            dcc_sessions.slots[slot].sin46.sin6.sin6_port *= 10;
            dcc_sessions.slots[slot].sin46.sin6.sin6_port += *ptr - '0';
        }
        dcc_sessions.slots[slot].sin46.sin6.sin6_port = htons(dcc_sessions.slots[slot].sin46.sin6.sin6_port);
    }

    int sock_fd = socket(dcc_sessions.slots[slot].sin46.sin_family, SOCK_STREAM, 0);

    if (sock_fd < 0) {
        perror("socket");
        goto close_fd;
    }

    int reuse = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        /* no need to return */ /* use socket as-is */
    }

    if (bind(sock_fd, (const struct sockaddr *)&dcc_sessions.slots[slot].sin46,
                         (dcc_sessions.slots[slot].sin46.sin_family == AF_INET) ? sizeof(struct sockaddr_in) :
                                                                                  sizeof(struct sockaddr_in6)) < 0) {
        perror("bind");
        goto close_socket;
    }

    if (listen(sock_fd, BACKLOG) < 0) {
        perror("listen");
        goto close_socket;
    }

    int flags = fcntl(sock_fd, F_GETFL, 0) | O_NONBLOCK;
    if (flags < 0 || fcntl(sock_fd, F_SETFL, flags) < 0) {
        perror("fcntl");
        goto close_socket;
    }

    struct stat statbuf;

    if (fstat(file_fd, &statbuf) < 0) {
        perror("fstat");
        goto close_socket;
    }

    dcc_sessions.slots[slot].file_size = statbuf.st_size;

    raw("PRIVMSG %s :\001DCC SEND %s %s %s %llu\001\r\n", target, dcc_sessions.slots[slot].filename, ip_addr_string, tok, statbuf.st_size);

    dcc_sessions.sock_fds[slot].fd = sock_fd;

    if (poll(&dcc_sessions.sock_fds[slot], 1, POLL_TIMEOUT) <= 0) { /* three minutes untill timeout */
        perror("poll");
        slot_clear(slot);
        goto close_socket;
    }

    int _sock_fd = accept(sock_fd, NULL, NULL);
    if (_sock_fd == -1) {
        perror("accept");
        slot_clear(slot);
        goto close_socket;
    }

    dcc_sessions.slots[slot].write = sock_fd + 1;
    dcc_sessions.sock_fds[slot].fd = _sock_fd;
    dcc_sessions.slots[slot].file_fd = file_fd;
    return;
close_socket:
    close(sock_fd);
close_fd:
    close(file_fd);
}

static inline void chan_privmsg(state l, char *channel, int offset)
{
    if(l->nick_privmsg == 0) {
        raw("PRIVMSG #%s :%s\r\n", channel, l->buf + offset);
        printf("\x1b[35mprivmsg #%s :%s\x1b[0m\r\n", channel, l->buf + offset);
    }
    else {
        raw("PRIVMSG %s :%s\r\n", channel, l->buf + offset);
        printf("\x1b[35mprivmsg %s :%s\x1b[0m\r\n", channel, l->buf + offset);
    }
}

static inline void chan_privmsg_command(state l)
{
    if (l->chan_privmsg) {
        l->chan_privmsg = 0;
        printf("\x1b[35mraw privmsg disabled\x1b[0m\r\n");
    }
    else {
        l->chan_privmsg = 1;
        printf("\x1b[35mraw privmsg enabled\x1b[0m\r\n");
    }
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
    if (!memcmp(l->buf, "/CHAN_PRIVMSG", sizeof("/CHAN_PRIVMSG") - 1) || !memcmp(l->buf, "/chan_privmsg", sizeof("/chan_privmsg") - 1)) {
        chan_privmsg_command(l);
        return;
    }
    if (l->chan_privmsg) {
        chan_privmsg(l, chan, 0);
        return;
    }
    switch (l->buf[0]) {
    case '/':           /* send system command */
        if (!memcmp(l->buf + 1, "JOIN", sizeof("JOIN") - 1) || !memcmp(l->buf + 1, "join", sizeof("join") - 1)) {
            join_command(l);
            return;
        }
        if (!memcmp(l->buf + 1, "PART", sizeof("PART") - 1) || !memcmp(l->buf + 1, "part", sizeof("part") - 1)) {
            part_command(l);
            return;
        }
        if (l->buf[1] == '/') {
            chan_privmsg(l, chan, sizeof("//") - 1);
            return;
        }
        if (!memcmp(l->buf + 1, "MSG", sizeof("MSG") - 1) || !memcmp(l->buf + 1, "msg", sizeof("msg") - 1)) {
            msg_command(l);
            return;
        }
        if (!memcmp(l->buf + 1, "NICK", sizeof("NICK") - 1) || !memcmp(l->buf + 1, "nick", sizeof("nick") - 1)) {
            nick_command(l);
            return;
        }
        if (!memcmp(l->buf + 1, "ACTION", sizeof("ACTION") - 1) || !memcmp(l->buf + 1, "action", sizeof("action") - 1)) {
            action_command(l);
            return;
        }
        if (!memcmp(l->buf + 1, "QUERY", sizeof("QUERY") - 1) || !memcmp(l->buf + 1, "query", sizeof("query") - 1)) {
            query_command(l);
            return;
        }
        if (!memcmp(l->buf + 1, "DCC", sizeof("DCC") - 1) || !memcmp(l->buf + 1, "dcc", sizeof("dcc") - 1)) {
            dcc_command(l);
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
        chan_privmsg(l, chan, 0);
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

static void slot_process(state l, char *buf, size_t buf_len, size_t i) {
    const int _write = dcc_sessions.slots[i].write;
    const char *err_str;

    const int sock_fd = dcc_sessions.sock_fds[i].fd;
    const int file_fd = dcc_sessions.slots[i].file_fd;

    if (!(dcc_sessions.sock_fds[i].revents & POLLIN) && !_write) {
        return;
    }

    int n = read(_write ? file_fd : sock_fd, buf, buf_len);
    if (n == -1) {
        err_str = "read";
        goto handle_err;
    }

    if ((n == 0) && !_write) { /* EOF */
        goto close_fd;
    }

    dcc_sessions.slots[i].bytes_read += n;
    if (write(_write ? sock_fd : file_fd, buf, n) < 0) {
        err_str = "write";
        goto handle_err;
    }

    if (!(dcc_sessions.sock_fds[i].revents & POLLOUT) && !_write) {
        refresh_line(l);
        return;
    }
    unsigned long long file_size = dcc_sessions.slots[i].file_size;
    unsigned long long bytes_read = dcc_sessions.slots[i].bytes_read;
    unsigned ack_is_64 = file_size > UINT_MAX;
    unsigned ack_shift = (1 - ack_is_64) * 32;
    unsigned long long ack = htonll(bytes_read << ack_shift);

    if ((_write ? read(sock_fd, &ack, ack_is_64 ? 8 : 4) : write(sock_fd, &ack, ack_is_64 ? 8 : 4))< 0) {
        err_str = _write ? "read" : "write";
        goto handle_err;
    }

    refresh_line(l);

    if (ack == htonll(file_size << ack_shift)) {
        goto close_fd;
    }
    return;
handle_err:
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
    }
    if (errno == ECONNRESET) {
        goto close_fd;
    }
    perror(err_str);
    dcc_sessions.slots[i].err_cnt++;
    if (dcc_sessions.slots[i].err_cnt > ERR_MAX) {
        goto close_fd;
    }
    return;
close_fd:
    shutdown(sock_fd, SHUT_RDWR);
    if (_write) {
        shutdown(_write - 1, SHUT_RDWR);
        close(_write - 1);
    }
    close(sock_fd);
    close(file_fd);
    slot_clear(i);
}

int main(int argc, char **argv)
{
    char buf[BUFSIZ];
    int cval;
    while ((cval = getopt(argc, argv, "s:p:o:n:k:c:u:r:a:D:46dfexvV")) != -1) {
        switch (cval) {
        case 'v':
            version();
            break;
        case 'V':
            verb = 1;
            break;
        case '4':
            hint_family = AF_INET;
            break;
        case '6':
            hint_family = AF_INET6;
            break;
        case 'e':
            sasl = 1;
            break;
        case 'd':
            dcc = 1;
            break;
        case 'f':
            filter = 1;
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
            if (strlen(optarg) < sizeof(chan)) {
                strcpy(chan, optarg);
            }
            else {
                print_error("argument to -c is too big");
            }
            break;
        case 'D':
            dcc_dir = optarg;
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
    if (dcc) {
        sigaction(SIGPIPE, &(struct sigaction){.sa_handler = SIG_IGN}, NULL);
    }
    if (cmds) {
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
    state_reset(&l);
    int rc, editReturnFlag = 0;
    if (enable_raw_mode(ttyinfd) == -1) {
        return 1;
    }
    if (setIsu8_C(ttyinfd, STDOUT_FILENO) == -1) {
        return 1;
    }
    for (;;) {
        if (poll(dcc_sessions.sock_fds, CON_MAX + 2, -1) == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            perror("poll");
            return 1;
        }
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
        if (dcc) {
            for (int i = 0; i < CON_MAX; i++) {
                slot_process(&l, buf, sizeof(buf), i);
            }
        }
    }
}
