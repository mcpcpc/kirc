#include "utf8.h"

static int utf8_get_cursor_position(int input_fd, int output_fd)
{
    char buf[32];
    int rows, cols;
    unsigned int i = 0;

    if (write(output_fd, "\x1b[6n", 4) != 4) {
        return -1;
    }

    while (i < sizeof(buf) - 1) {
        ssize_t n = read(input_fd, buf + i, 1);
        if (n != 1) {
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

int utf8_detect(utf8_context_t *utf8, int input_fd, int output_fd)
{
    static const char *test_chars[] = {
        "\xe1\xbb\xa4", /* some wide-ish UTF-8 char */
        NULL
    };

    if (!utf8) {
        return -1;
    }
    if (utf8->enabled) {
        return 0;
    }

    /* Move to column 1 so we can test width reliably. */
    if (write(output_fd, "\r", 1) != 1) {
        return -1;
    }
    if (utf8_get_cursor_position(input_fd, output_fd) != 1) {
        return -1;
    }

    for (const char **it = test_chars; *it; ++it) {
        size_t len = strlen(*it);

        if (write(output_fd, *it, len) != (ssize_t)len) {
            return -1;
        }

        int pos = utf8_get_cursor_position(input_fd, output_fd);
        if (pos == -1) {
            return -1;
        }

        /* Clear what we just drew: CR, then spaces, then CR again. */
        if (write(output_fd, "\r", 1) != 1) {
            return -1;
        }
        for (int i = 1; i < pos; ++i) {
            if (write(output_fd, " ", 1) != 1) {
                return -1;
            }
        }
        if (write(output_fd, "\r", 1) != 1) {
            return -1;
        }

        /* Expect the cursor to advance by 2 columns for this wide char.
         * If not, we treat as "no UTF-8" but *not* as an error.
         */
        if (pos != 2) {
            return 0;
        }
    }

    utf8->enabled = 1;
    return 0;
}

int utf8_is_char_start(utf8_context_t *utf8, char c)
{
    /* If UTF-8 not enabled, every byte is treated as a char boundary. */
    if (!utf8 || !utf8->enabled) {
        return 1;
    }

    return ((c & 0x80) == 0x00) || ((c & 0xC0) == 0xC0);
}

int utf8_char_size(utf8_context_t *utf8, char c)
{
    int size = 1;

    if (!utf8 || !utf8->enabled) {
        return 1;
    }

    /* Count leading 1 bits. */
    size = 0;
    while (c & (0x80 >> size)) {
        size++;
    }

    if (size == 0) {
        size = 1;
    }

    return size;
}

size_t utf8_strlen(utf8_context_t *utf8, const char *s)
{
    size_t lenu8 = 0;

    if (!s) {
        return 0;
    }

    while (*s != '\0') {
        lenu8 += utf8_is_char_start(utf8, *s);
        s++;
    }

    return lenu8;
}

size_t utf8_prev(utf8_context_t *utf8, const char *s,
        size_t byte_pos)
{
    if (!s) {
        return 0;
    }

    if (byte_pos != 0) {
        do {
            byte_pos--;
        } while (byte_pos > 0 && !utf8_is_char_start(utf8, s[byte_pos]));
    }

    return byte_pos;
}

size_t utf8_next(utf8_context_t *utf8, const char *s,
        size_t byte_pos)
{
    if (!s) {
        return 0;
    }

    if (s[byte_pos] != '\0') {
        do {
            byte_pos++;
        } while (s[byte_pos] != '\0' &&
                 !utf8_is_char_start(utf8, s[byte_pos]));
    }

    return byte_pos;
}
