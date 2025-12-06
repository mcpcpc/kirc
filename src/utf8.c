#include "utf8.h"

int utf8_detect(utf8_t *u, int in_fd, int out_fd)
{
    if (!u) {
        return -1;
    }

    if (u->enabled) {
        return 0;
    }

    /* Reset cursor to column 1 */
    if (write(out_fd, "\r", 1) != 1) {
        return -1;
    }

    /* Query cursor position */
    if (write(out_fd, "\x1b[6n", 4) != 4) {
        return -1;
    }

    char buf[32];
    unsigned int i = 0;

    while (i < sizeof(buf) - 1) {
        if (read(in_fd, buf + i, 1) != 1) {
            break;
        }
        if (buf[i] == 'R') {
            break;
        }
        i++;
    }

    buf[i] = '\0';

    /* Expect ESC [ row ; col R */
    if (buf[0] != 27 || buf[1] != '[') {
        return -1;
    }

    int row = 0, col = 0;
    if (sscanf(buf + 2, "%d;%d", &row, &col) != 2) {
        return -1;
    }

    /* If we're not at column 1, the terminal is already odd */
    if (col != 1) {
        return 0;
    }

    const char *test_char = "\xe1\xbb\xa4";

    if (write(out_fd, test_char, strlen(test_char)) !=
        (ssize_t)strlen(test_char)) {
        return -1;
    }

    /* Query cursor again */
    if (write(out_fd, "\x1b[6n", 4) != 4) {
        return -1;
    }

    i = 0;
    while (i < sizeof(buf) - 1) {
        if (read(in_fd, buf + i, 1) != 1) {
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

    row = col = 0;
    if (sscanf(buf + 2, "%d;%d", &row, &col) != 2) {
        return -1;
    }

    /* Clean up cursor position */
    if (write(out_fd, "\r", 1) != 1) {
        return -1;
    }

    for (int j = 1; j < col; j++) {
        if (write(out_fd, " ", 1) != 1) {
            return -1;
        }
    }

    if (write(out_fd, "\r", 1) != 1) {
        return -1;
    }

    if (col == 2) {
        u->enabled = 1;
    } else {
        u->enabled = 0;
    }

    return 0;
}

int utf8_is_start(utf8_t *u, char c)
{
    if (!u || !u->enabled) {
        return 1;
    }

    return ((c & 0x80) == 0x00) || ((c & 0xC0) == 0xC0);
}

int utf8_char_size(utf8_t *u, char c)
{
    if (!u || !u->enabled) {
        return 1;
    }

    int size = 0;
    unsigned char uc = (unsigned char)c;

    while (uc & (0x80 >> size)) {
        size++;
    }

    return (size > 0) ? size : 1;
}

size_t utf8_length(utf8_t *u, const char *s)
{
    size_t len = 0;

    if (!s) {
        return 0;
    }

    while (*s) {
        len += utf8_is_start(u, *s);
        s++;
    }

    return len;
}

size_t utf8_prev(utf8_t *u, const char *s, size_t pos)
{
    if (!s || pos == 0) {
        return 0;
    }

    do {
        pos--;
    } while (pos > 0 && !utf8_is_start(u, s[pos]));

    return pos;
}

size_t utf8_next(utf8_t *u, const char *s, size_t pos)
{
    if (!s || s[pos] == '\0') {
        return pos;
    }

    do {
        pos++;
    } while (s[pos] != '\0' && !utf8_is_start(u, s[pos]));

    return pos;
}
