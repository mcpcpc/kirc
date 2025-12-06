#include "editor.h"

struct abuf {
    char *b;
    size_t len;
};

static void ab_init(struct abuf *ab)
{
    ab->b = NULL;
    ab->len = 0;
}

static void ab_append(struct abuf *ab, const char *s,
        size_t len)
{
    char *newbuf;

    if (len == 0) {
        return;
    }
    newbuf = realloc(ab->b, ab->len + len);
    if (!newbuf) {
        return;
    }
    memcpy(newbuf + ab->len, s, len);
    ab->b = newbuf;
    ab->len += len;
}

static void ab_free(struct abuf *ab)
{
    free(ab->b);
    ab->b = NULL;
    ab->len = 0;
}

static void buffer_position_move(editor_context_t *ectx,
        ssize_t dest, ssize_t src, size_t size)
{
    memmove(ectx->buf + ectx->posb + dest,
            ectx->buf + ectx->posb + src, size);
}

static void buffer_position_move_end(editor_context_t *ectx,
        ssize_t dest, ssize_t src)
{
    size_t tail = ectx->lenb - (ectx->posb + (size_t)src) + 1;
    buffer_position_move(ectx, dest, src, tail);
}

static void history_pop_placeholder(editor_context_t *ectx)
{
    history_context_t *hist = ectx->ctx->history;

    if (!hist || !hist->items || hist->length <= 0) {
        return;
    }

    free(hist->items[hist->length - 1]);
    hist->items[hist->length - 1] = NULL;
    hist->length--;
}

void editor_refresh(editor_context_t *ectx)
{
    char seq[64];
    char *buf = ectx->buf;
    size_t lenb = ectx->lenb;
    size_t posu8 = ectx->posu8;
    size_t plenu8 = ectx->plenu8 + 2; /* prompt + "> " */
    size_t ch = plenu8;
    size_t txtlenb = 0;
    struct abuf ab;

    if (!ectx->ctx->terminal) {
        return;
    }

    ectx->cols = (size_t)terminal_get_columns(ed->terminal, STDOUT_FILENO);

    /* Scroll horizontally if cursor would go beyond the right edge. */
    while ((plenu8 + posu8) >= ectx->cols && buf[0] != '\0') {
        buf += utf8_next(ectx->ctx->utf8, buf, 0);
        posu8--;
    }

    /* Compute visible byte length. */
    while (txtlenb < lenb && ++ch < ectx->cols) {
        txtlenb += utf8_next(ectx->ctx->utf8, buf + txtlenb, 0);
    }

    ab_init(&ab);

    /* Move cursor to column 0. */
    snprintf(seq, sizeof(seq), "\r");
    ab_append(&ab, seq, strlen(seq));

    /* Print prompt + "> " */
    ab_append(&ab, ectx->prompt, ectx->plenb);
    ab_append(&ab, "> ", 2);

    /* Print buffer contents. */
    ab_append(&ab, buf, txtlenb);

    /* Clear to end of line. */
    snprintf(seq, sizeof(seq), "\x1b[0K");
    ab_append(&ab, seq, strlen(seq));

    /* Reposition cursor. */
    if (posu8 + plenu8) {
        snprintf(seq, sizeof(seq), "\r\x1b[%dC",
                 (int)(posu8 + plenu8));
    } else {
        snprintf(seq, sizeof(seq), "\r");
    }
    ab_append(&ab, seq, strlen(seq));

    /* Write out. */
    (void)write(STDOUT_FILENO, ab.b, ab.len);

    ab_free(&ab);
}

static void edit_move_left(editor_context_t *ectx)
{
    if (ectx->posb > 0) {
        ectx->posb = utf8_prev(ectx->ctx->utf8, ectx->buf,
            ectx->posb);
        ectx->posu8--;
        editor_refresh(ectx);
    }
}

static void edit_move_right(editor_context_t *ectx)
{
    if (ectx->posu8 != ectx->lenu8) {
        ectx->posb = utf8_next(ectx->ctx->utf8, ectx->buf,
            ectx->posb);
        ectx->posu8++;
        editor_refresh(ectx);
    }
}

static void edit_move_home(editor_context_t *ectx)
{
    if (ectx->posb != 0) {
        ectx->posb = 0;
        ectx->posu8 = 0;
        editor_refresh(ectx);
    }
}

static void edit_move_end(editor_context_t *ectx)
{
    if (ectx->posu8 != ectx->lenu8) {
        ectx->posb = ectx->lenb;
        ectx->posu8 = ectx->lenu8;
        editor_refresh(ectx);
    }
}

static void edit_delete(editor_context_t *ectx)
{
    if ((ectx->lenu8 > 0) && (ectx->posu8 < ectx->lenu8)) {
        size_t next = utf8_next(ectx->ctx->utf8, ectx->buf,
            ectx->posb);
        size_t this_size = next - ectx->posb;

        buffer_position_move_end(ectx, 0, (ssize_t)this_size);
        ectx->lenb -= this_size;
        ectx->lenu8--;
        editor_refresh(ectx);
    }
}

static void edit_backspace(editor_context_t *ectx)
{
    if ((ectx->posu8 > 0) && (ectx->lenu8 > 0)) {
        size_t prev = utf8_prev(ectx->ctx->utf8, ectx->buf,
            ectx->posb);
        size_t prev_size = ectx->posb - prev;

        buffer_position_move_end(ectx, -(ssize_t)prev_size, 0);
        ectx->posb -= prev_size;
        ectx->lenb -= prev_size;
        ectx->posu8--;
        ectx->lenu8--;
        editor_refresh(ectx);
    }
}

static void edit_delete_previous_word(editor_context_t *ectx)
{
    size_t old_posb = ectx->posb;
    size_t old_posu8 = ectx->posu8;

    /* Skip trailing spaces. */
    while ((ectx->posb > 0) && (ectx->buf[ectx->posb - 1] == ' ')) {
        ectx->posb--;
        ectx->posu8--;
    }

    /* Delete word. */
    while ((ectx->posb > 0) && (ectx->buf[ectx->posb - 1] != ' ')) {
        if (utf8_is_char_start(ectx->ctx->utf8, ectx->buf[ectx->posb - 1])) {
            ectx->posu8--;
        }
        ectx->posb--;
    }

    size_t diffb = old_posb - ectx->posb;
    size_t diffu8 = old_posu8 - ectx->posu8;

    buffer_position_move_end(ectx, 0, (ssize_t)diffb);
    ectx->lenb -= diffb;
    ectx->lenu8 -= diffu8;
    editor_refresh(ectx);
}

static void edit_delete_whole_line(editor_context_t *ectx)
{
    ectx->buf[0] = '\0';
    ectx->posb = ectx->lenb = ectx->posu8 = ectx->lenu8 = 0;
    editor_refresh(ectx);
}

static void edit_delete_line_to_end(editor_context_t *ectx)
{
    ectx->buf[ectx->posb] = '\0';
    ectx->lenb = ectx->posb;
    ectx->lenu8 = ectx->posu8;
    editor_refresh(ectx);
}

static int edit_insert(editor_context_t *ectx, const char *c)
{
    size_t clenb = strlen(c);

    if ((ectx->lenb + clenb) >= ectx->buflen) {
        return 0;
    }

    if (ectx->lenu8 == ectx->posu8) {
        /* Append at end. */
        strcpy(ectx->buf + ectx->posb, c);
        ectx->posu8++;
        ectx->lenu8++;
        ectx->posb += clenb;
        ectx->lenb += clenb;

        if ((ectx->plenu8 + ectx->lenu8) < ectx->cols) {
            if (write(STDOUT_FILENO, c, clenb) == -1) {
                return -1;
            }
        } else {
            editor_refresh(ectx);
        }
    } else {
        /* Insert in the middle. */
        buffer_position_move_end(ectx, (ssize_t)clenb, 0);
        memmove(ectx>buf + ectx->posb, c, clenb);
        ectx->posu8++;
        ectx->lenu8++;
        ectx->posb += clenb;
        ectx->lenb += clenb;
        editor_refresh(ectx);
    }

    return 0;
}

static void edit_history(editor_context_t *ectx, int dir)
{
    history_context_t *hist = ectx->history;

    if (!hist || !hist->items || hist->length <= 1) {
        return;
    }

    /* Save current buffer into current history entry. */
    int cur_index = hist->length - (1 + ed->history_index);
    if (cur_index < 0 || cur_index >= hist->length) {
        return;
    }

    free(hist->items[cur_index]);
    hist->items[cur_index] = strdup(ectx->buf);
    if (!hist->items[cur_index]) {
        return;
    }

    ectx->history_index += (dir == 1) ? 1 : -1;

    if (ectx->history_index < 0) {
        ectx->history_index = 0;
        return;
    } else if (ectx->history_index >= hist->length) {
        ectx->history_index = hist->length - 1;
        return;
    }

    int new_index = hist->length - (1 + ectx->history_index);
    if (new_index < 0 || new_index >= hist->length) {
        return;
    }

    /* Load new history entry into buffer. */
    strncpy(ectx->buf, hist->items[new_index], ectx->buflen);
    ectx->buf[ectx->buflen - 1] = '\0';

    ectx->lenb = ectx->posb = strlen(ectx->buf);
    ectx->lenu8 = ectx->posu8 = utf8_strlen(ectx->utf8,
        ectx->buf);

    editor_refresh(ectx);
}

static void edit_escape_sequence(editor_context_t *ectx,
        char seq[3])
{
    int fd = ectx->ctx->terminal->tty_fd;

    if (read(fd, seq, 1) == -1) {
        return;
    }
    if (read(fd, seq + 1, 1) == -1) {
        return;
    }

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(fd, seq + 2, 1) == -1) {
                return;
            }
            if (seq[2] == '~' && seq[1] == '3') {
                edit_delete(ectx); /* Delete key */
            }
        } else {
            switch (seq[1]) {
            case 'A':  /* Up */
                edit_history(ectx, 1);
                break;

            case 'B':  /* Down */
                edit_history(ectx, 0);
                break;

            case 'C':  /* Right */
                edit_move_right(ectx);
                break;

            case 'D':  /* Left */
                edit_move_left(ectx);
                break;

            case 'H':  /* Home */
                edit_move_home(ectx);
                break;

            case 'F':  /* End */
                edit_move_end(ectx);
                break;
            }
        }
    } else if (seq[0] == 'O') {
        switch (seq[1]) {
        case 'H':
            edit_move_home(ectx);
            break;

        case 'F':
            edit_move_end(ectx);
            break;
        }
    }
}

void editor_init(editor_context_t *ectx, kirc_context_t *ctx,
        char *buffer, size_t buflen, char *prompt)
{
    if (!ectx) {
        return;
    }

    ectx->ctx = ctx;

    ectx->buf = buffer;
    ectx->buflen = buflen;
    ectx->prompt = prompt;

    ectx->cols = 0;
    ectx->history_index = 0;

    editor_reset(ectx);
}

void editor_reset(editor_context_t *ectx)
{
    if (!ectx) {
        return;
    }

    if (!ectx->prompt) {
        ectx->prompt = "";
    }

    ectx->plenb = strlen(ectx->prompt);
    ectx->plenu8 = utf8_strlen(ectx->ctx->utf8, ectx->prompt);

    ectx->oldposb = 0;
    ectx->posb = 0;
    ectx->oldposu8 = 0;
    ectx->posu8 = 0;
    ectx->lenb = 0;
    ectx->lenu8 = 0;
    ectx->history_index = 0;

    if (ectx->buf && ectx->buflen > 0) {
        ectx->buf[0] = '\0';
    }

    /* Add placeholder entry to history (mirrors original state_reset). */
    if (ectx->ctx->history) {
        history_add(ectx->ctx->history, "");
    }
}

int editor_process_keypress(editor_context_t *ectx)
{
    char c;
    char seq[3];
    int ret = 0;
    int fd = ectx->ctx->terminal->tty_fd;

    ssize_t nread = read(fd, &c, 1);
    if (nread <= 0) {
        /* Preserve original behavior: treat as "done" and let caller decide. */
        ret = 1;
    } else {
        switch (c) {
        case 13: /* Enter */
            history_pop_placeholder(ectx);
            return 1;
        case 3:  /* Ctrl-C */
            errno = EAGAIN;
            return -1;
        case 127:  /* Backspace */
        case 8:    /* Ctrl-H */
            edit_backspace(ectx);
            break;
        case 2:  /* Ctrl-B */
            edit_move_left(ectx);
            break;
        case 6:  /* Ctrl-F */
            edit_move_right(ectx);
            break;
        case 1:  /* Ctrl-A */
            edit_move_home(ectx);
            break;
        case 5:  /* Ctrl-E */
            edit_move_end(ectx);
            break;
        case 23: /* Ctrl-W */
            edit_delete_previous_word(ectx);
            break;
        case 21: /* Ctrl-U */
            edit_delete_whole_line(ectx);
            break;
        case 11: /* Ctrl-K */
            edit_delete_line_to_end(ectx);
            break;
        case 14: /* Ctrl-N: newer history */
            edit_history(ectx, 0);
            break;
        case 16: /* Ctrl-P: older history */
            edit_history(ectx, 1);
            break;
        case 27: /* ESC: escape sequence */
            edit_escape_sequence(ectx, seq);
            break;
        case 4:  /* Ctrl-D */
            if (ectx->lenu8 > 0) {
                edit_delete(ectx);
            } else {
                /* Remove placeholder and request exit. */
                history_pop_placeholder(ectx);
                ret = -1;
            }
            break;
        default:
            if (utf8_is_char_start(ectx->ctx->utf8, c)) {
                char aux[8];
                int size;
                int i;

                aux[0] = c;
                size = utf8_char_size(ectx->ctx->utf8, c);

                for (i = 1; i < size; ++i) {
                    nread = read(fd, aux + i, 1);
                    if (nread != 1 || (aux[i] & 0xC0) != 0x80) {
                        break;
                    }
                }
                aux[size] = '\0';

                if (edit_insert(ectx, aux)) {
                    ret = -1;
                }
            }
            break;
        }
    }

    return ret;
}
