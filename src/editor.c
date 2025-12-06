#include "editor.h"

static void buffer_move(editor_t *e, ssize_t dest,
        ssize_t src, size_t size)
{
    memmove(e->buffer + e->cursor_byte + dest,
        e->buffer + e->cursor_byte + src, size);
}

static void
buffer_move_end(editor_t *e, ssize_t dest, ssize_t src)
{
    buffer_move(e, dest, src,
        e->content_len_bytes - (e->cursor_byte + src) + 1);
}

void editor_reset(editor_t *e)
{
    if (!e || !e->buffer) {
        return;
    }

    e->prompt_len_bytes = strlen(e->prompt);
    e->prompt_len_u8 = utf8_length(e->utf8, e->prompt);

    e->cursor_byte = 0;
    e->cursor_u8 = 0;
    e->prev_cursor_byte = 0;
    e->prev_cursor_u8 = 0;

    e->content_len_bytes = 0;
    e->content_len_u8 = 0;

    e->history_index = 0;

    e->terminal_cols = terminal_columns(e->terminal);

    e->buffer[0] = '\0';
}

static void editor_insert(editor_t *e, const char *s)
{
    size_t len = strlen(s);

    if (e->content_len_bytes + len >= e->buffer_size) {
        return;
    }

    if (e->cursor_u8 == e->content_len_u8) {
        memcpy(e->buffer + e->cursor_byte, s, len + 1);
    } else {
        buffer_move_end(e, (ssize_t)len, 0);
        memcpy(e->buffer + e->cursor_byte, s, len);
    }

    e->cursor_byte += len;
    e->content_len_bytes += len;

    e->cursor_u8++;
    e->content_len_u8++;
}

static void editor_move_left(editor_t *e)
{
    if (e->cursor_byte > 0) {
        e->cursor_byte =
            utf8_prev(e->utf8, e->buffer, e->cursor_byte);
        e->cursor_u8--;
    }
}

static void editor_move_right(editor_t *e)
{
    if (e->cursor_u8 < e->content_len_u8) {
        e->cursor_byte =
            utf8_next(e->utf8, e->buffer, e->cursor_byte);
        e->cursor_u8++;
    }
}

static void editor_move_home(editor_t *e)
{
    e->cursor_byte = 0;
    e->cursor_u8 = 0;
}

static void editor_move_end(editor_t *e)
{
    e->cursor_byte = e->content_len_bytes;
    e->cursor_u8   = e->content_len_u8;
}

static void editor_delete(editor_t *e)
{
    if (e->cursor_u8 < e->content_len_u8) {
        size_t next =
            utf8_next(e->utf8, e->buffer, e->cursor_byte);
        size_t size = next - e->cursor_byte;

        buffer_move_end(e, 0, (ssize_t)size);

        e->content_len_bytes -= size;
        e->content_len_u8--;
    }
}

static void editor_backspace(editor_t *e)
{
    if (e->cursor_u8 > 0) {
        size_t prev =
            utf8_prev(e->utf8, e->buffer, e->cursor_byte);
        size_t size = e->cursor_byte - prev;

        buffer_move_end(e, -(ssize_t)size, 0);

        e->cursor_byte -= size;
        e->content_len_bytes -= size;

        e->cursor_u8--;
        e->content_len_u8--;
    }
}

static void editor_delete_whole_line(editor_t *e)
{
    e->buffer[0] = '\0';
    e->cursor_byte = 0;
    e->cursor_u8 = 0;
    e->content_len_bytes = 0;
    e->content_len_u8 = 0;
}

static void editor_delete_to_end(editor_t *e)
{
    e->buffer[e->cursor_byte] = '\0';
    e->content_len_bytes = e->cursor_byte;
    e->content_len_u8 = e->cursor_u8;
}

static void editor_history(editor_t *e, int direction)
{
    if (!e->history || e->history->length < 1) {
        return;
    }

    e->history_index += (direction > 0) ? 1 : -1;

    if (e->history_index < 0) {
        e->history_index = 0;
        return;
    }

    const char *entry = history_get(e->history,
        e->history_index);

    if (!entry) {
        e->history_index--;
        return;
    }

    strncpy(e->buffer, entry, e->buffer_size - 1);
    e->buffer[e->buffer_size - 1] = '\0';

    e->content_len_bytes = strlen(e->buffer);
    e->content_len_u8 =
        utf8_length(e->utf8, e->buffer);

    editor_move_end(e);
}

static void editor_escape(editor_t *e)
{
    char seq[3];

    if (read(e->terminal->tty_fd, &seq[0], 1) != 1)
        return;
    if (read(e->terminal->tty_fd, &seq[1], 1) != 1)
        return;

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(e->terminal->tty_fd, &seq[2], 1) != 1)
                return;
            if (seq[1] == '3' && seq[2] == '~') {
                editor_delete(e);
            }
        } else {
            switch (seq[1]) {
            case 'A':
                editor_history(e,  1);
                break;

            case 'B': 
                editor_history(e, -1);
                break;

            case 'C':
                editor_move_right(e);
                break;

            case 'D':
                editor_move_left(e);
                break;

            case 'H':
                editor_move_home(e);
                break;

            case 'F':
                editor_move_end(e);
                break;
            }
        }
    }
}

int editor_process_key(editor_t *e)
{
    char c;

    ssize_t n = read(e->terminal->tty_fd, &c, 1);
    if (n <= 0) {
        return 1;
    }

    switch (c) {
    case 13:  /* ENTER */
        history_add(e->history, e->buffer);
        return 1;

    case 3:  /* CTRL-C */
        errno = EAGAIN;
        return -1;

    case 127:  /* BACKSPACE */
    case 8:
        editor_backspace(e);
        break;

    case 2:
        editor_move_left(e);
        break;

    case 6:
        editor_move_right(e);
        break;

    case 1:
        editor_move_home(e);
        break;

    case 5:
        editor_move_end(e);
        break;

    case 21:
        editor_delete_whole_line(e);
        break;

    case 11:
        editor_delete_to_end(e);
        break;

    case 14:
        editor_history(e, -1);
        break;

    case 16:
        editor_history(e,  1);
        break;

    case 27:
        editor_escape(e);
        break;

    case 4: /* CTRL-D */
        if (e->content_len_u8 > 0) {
            editor_delete(e);
        } else {
            return -1;
        }
        break;

    default:
        if (utf8_is_start(e->utf8, c)) {
            char tmp[8];
            tmp[0] = c;

            int size = utf8_char_size(e->utf8, c);

            for (int i = 1; i < size; i++) {
                if (read(e->terminal->tty_fd, &tmp[i], 1) != 1)
                    return 0;
            }

            tmp[size] = '\0';
            editor_insert(e, tmp);
        }
        break;
    }

    return 0;
}
