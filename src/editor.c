#include "editor.h"

static void editor_scratch_append(editor_t *editor, const char *value)
{
    int scratch_max = editor->scratch_max;
    int scratch_size = editor->scratch_size;
    int loc = (scratch_size + 1) % scratch_max;
    int loc_n = sizeof(editor->scratch[loc]) - 1;

    strncpy(editor->scratch[loc], value, loc_n);
}

static void editor_backspace(editor_t *editor)
{
}

static void editor_enter(editor_t *editor)
{
}

static void editor_delete(editor_t *editor)
{
}

static void editor_history(editor_t *editor, int dir)
{
}

static void editor_move_right(editor_t *editor)
{
}

static void editor_move_left(editor_t *editor)
{
}

static void editor_move_home(editor_t *editor)
{
}

static void editor_move_end(editor_t *editor)
{
}

static void editor_escape(editor_t *editor)
{
    char seq[3];
    int tty_fd = editor->ctx->tty_fd;

    if (read(tty_fd, &seq[0], 1) != 1) {
        return;
    }

    if (read(tty_fd, &seq[1], 1) != 1) {
        return;
    }

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(tty_fd, &seq[2], 1) != 1) {
                return;
            }

            if (seq[1] == '3' && seq[2] == '~') {
                editor_delete(editor);
            }
        } else {
            switch (seq[1]) {
            case 'A':
                editor_history(editor, 1);
                break;

            case 'B':
                editor_history(editor, -1);
                break;

            case 'C':
                editor_move_right(editor);
                break;

            case 'D':
                editor_move_left(editor);
                break;

            case 'H':
                editor_move_home(editor);
                break;

            case 'F':
                editor_move_end(editor);
                break;
            }
        }
    }
}

static void editor_insert(editor_t *editor, char c)
{
}

int editor_init(editor_t *editor, kirc_t *ctx)
{
    memset(editor, 0, sizeof(*editor));

    editor->ctx = ctx;
    editor->scratch_max = KIRC_SCRATCH_MAX;
    editor->scratch_size = 0;
    editor->scratch_current = 0;
    editor->scratch_cursor = 0;

    return 0;
}

int editor_process_key(editor_t *editor)
{
    char c;
    int tty_fd = editor->ctx->tty_fd;
    
    if (read(tty_fd, &c, 1) < 1) {
        return 1; 
    }

    switch(c) {
    case 3:  /* CTRL-C */
        errno = EAGAIN;
        return -1;

    case 127:  /* ESCAPE */
    case 8:  /* DEL */ 
        editor_backspace(editor);
        break;

    case 13:  /* ENTER */
        editor_enter(editor);
        break;

    case 27:  /* ESCAPE */
        editor_escape(editor);
        break;

    default:
        editor_insert(editor, c);
        break;
    }

    return 0;
}

int editor_render(editor_t *editor)
{
    int cols = terminal_columns(editor->ctx);
    int scratch_max = editor->scratch_max;
    int scratch_size = editor->scratch_size;
    int loc = (scratch_size + 1) % scratch_max;
    int offset = cols - 1;
    printf("\r:%.*s\x1b[0K", offset, editor->scratch[loc]);

    if (editor->scratch_cursor > 0) {
        printf("\r\x1b[%dC", editor->scratch_cursor + 1);
    } else {
        //printf("\r");
        printf("\r\x1b[%dC", 1);
    }

    fflush(stdout);
    return 0;
}
