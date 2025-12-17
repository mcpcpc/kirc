#include "editor.h"

static void editor_backspace(editor_t *editor)
{
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    if (editor->cursor - 1 < 0) {
        return;  /* nothing to delete or out of range */
    }

    editor->cursor--;

    memmove(editor->scratch + editor->cursor,
        editor->scratch + editor->cursor + 1,
        len - editor->cursor);
}

static void editor_delete_whole_line(editor_t *editor)
{
    editor->scratch[0] = '\0';
    editor->cursor = 0;
}

static void editor_enter(editor_t *editor)
{
    int siz = sizeof(editor->scratch) - 1;

    strncpy(editor->history[editor->head],
        editor->scratch, siz);

    editor->head = (editor->head + 1) % KIRC_HISTORY_SIZE;

    editor_delete_whole_line(editor);
}

static void editor_delete(editor_t *editor)
{
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    if (editor->cursor > len) {
        return;  /* at end of scratch string */
    }

    memmove(editor->scratch + editor->cursor,
        editor->scratch + editor->cursor + 1,
        len - editor->cursor);
}

static void editor_history(editor_t *editor, int dir)
{
    // placeholder
}

static void editor_move_right(editor_t *editor)
{
    int siz = sizeof(editor->scratch) - 1;
 
    if (editor->cursor > siz) {
        return;  /* at end of scratch */
    }

    int len = strlen(editor->scratch);

    if (editor->cursor + 1 > len) {
        return;
    }

    editor->cursor++;
}

static void editor_move_left(editor_t *editor)
{
    if (editor->cursor - 1 < 0) {
        return;
    }

    editor->cursor--;
}

static void editor_move_home(editor_t *editor)
{
    editor->cursor = 0;
}

static void editor_move_end(editor_t *editor)
{
    size_t siz = sizeof(editor->scratch) - 1;

    editor->cursor = strnlen(editor->scratch, siz);
}

static void editor_escape(editor_t *editor)
{
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) {
        return;
    }

    if (read(STDIN_FILENO, &seq[1], 1) != 1) {
        return;
    }

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(STDIN_FILENO, &seq[2], 1) != 1) {
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
    int siz = sizeof(editor->scratch) - 1;

    if (editor->cursor > siz) {
        return;  /* at end of scratch */
    }

    int len = strlen(editor->scratch);

    if (len + 1 >= siz) {
        return;  /* scratch full */
    }

    memmove(editor->scratch + editor->cursor + 1,
        editor->scratch + editor->cursor,
        len - editor->cursor + 1);

    editor->scratch[editor->cursor] = c;
    editor->cursor++;
}

static void editor_clear(editor_t *editor)
{
    printf("\r\x1b[0K");
}

static void editor_toggle_filter(editor_t *editor)
{
    editor->ctx->filtered = !editor->ctx->filtered;
}

int editor_init(editor_t *editor, kirc_t *ctx)
{
    memset(editor, 0, sizeof(*editor));
    
    editor->ctx = ctx;
    editor->count = 0;
    editor->cursor = 0;
    editor->head = 0;

    return 0;
}

int editor_process_key(editor_t *editor)
{
    char c;
    
    if (read(STDIN_FILENO, &c, 1) < 1) {
        return 1;
    }

    switch(c) {
    case 3:  /* CTRL-C */
        errno = EAGAIN;
        editor_clear(editor);
        return -1;

    case 21:  /* CTRL-U */
        editor_delete_whole_line(editor);
        break;

    case 17: /* CTRL-Q */
        editor_toggle_filter(editor);
        break;

    case 127:  /* DELETE */
        editor_backspace(editor);
        break;

    case 13:  /* ENTER */
        editor_enter(editor);
        return 1;

    case 27:  /* ESCAPE */
        editor_escape(editor);
        break;

    default:
        editor_insert(editor, c);
        break;
    }

    return 0;
}

/*
int editor_render(editor_t *editor)
{
    int cols = terminal_columns(STDIN_FILENO);
    int start = editor->cursor - (cols - 1) < 0 ?
        0 : editor->cursor - (cols - 1);

    printf("\r\x1b[7m%.*s\x1b[0m \x1b[0K", cols - 1,
        editor->scratch + start);

    if (editor->cursor > 0) {
        printf("\r\x1b[%dC", editor->cursor);
    } else {
        printf("\r");
    }

    fflush(stdout);

    return 0;
}
*/

int editor_render(editor_t *editor)
{
    int cols = terminal_columns(STDIN_FILENO);
    int size = strlen(editor->ctx->selected) + 1;
    int start = editor->cursor - (cols - size - 1) < 0 ?
        0 : editor->cursor - (cols - size - 1);

    printf("\r\x1b[7;34m%s%c\x1b[0m\x1b[7m%.*s\x1b[0m \x1b[0K",
        editor->ctx->selected,
        editor->ctx->filtered ? '~' : ':',
        cols - size - 1, editor->scratch + start);

    printf("\r\x1b[%dC", editor->cursor + size);

    fflush(stdout);

    return 0;
}