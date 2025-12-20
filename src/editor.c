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

static void editor_delete_line(editor_t *editor)
{
    editor->scratch[0] = '\0';
    editor->cursor = 0;
}

static int editor_enter(editor_t *editor)
{
    if (editor->scratch[0] == '\0') {
        return 0;  /* nothing to send */
    }

    int siz = sizeof(editor->scratch) - 1;

    strncpy(editor->history[editor->head],
        editor->scratch, siz);

    editor->head = (editor->head + 1) % KIRC_HISTORY_SIZE;
    if (editor->count < KIRC_HISTORY_SIZE) {
        editor->count++;
    }
    editor->position = -1;

    editor_delete_line(editor);
    
    return 1;
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
    int siz = KIRC_HISTORY_SIZE;

    if (editor->count == 0) {
        return;
    }

    int head = editor->head;
    int oldest = (head - editor->count + siz) % siz;
    int newest = (head - 1 + siz) % siz;

    if (dir > 0) {  /* up or previous */
        int pos;
        if (editor->position == -1) {
            pos = newest;
        } else if (editor->position == oldest) {
            return; /* already at oldest */
        } else {
            pos = (editor->position - 1 + siz) % siz;
        }

        strncpy(editor->scratch, editor->history[pos],
            sizeof(editor->scratch) - 1);
        editor->scratch[sizeof(editor->scratch) - 1] = '\0';
        editor->cursor = strnlen(editor->scratch,
            sizeof(editor->scratch) - 1);
        editor->position = pos;
    } else { /* down or next */
        if (editor->position == -1) {
            return; /* not browsing */
        }

        if (editor->position == newest) {
            editor_delete_line(editor);
            editor->position = -1;
            return;
        }

        int pos = (editor->position + 1) % siz;

        strncpy(editor->scratch, editor->history[pos],
            sizeof(editor->scratch) - 1);
        editor->scratch[sizeof(editor->scratch) - 1] = '\0';
        editor->cursor = strnlen(editor->scratch,
            sizeof(editor->scratch) - 1);
        editor->position = pos;
    }
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

char *editor_last_entry(editor_t *editor)
{
    int head = editor->head;
    int last = (head - 1 + KIRC_HISTORY_SIZE) % KIRC_HISTORY_SIZE;

    return editor->history[last];
}

int editor_init(editor_t *editor, kirc_t *ctx)
{
    memset(editor, 0, sizeof(*editor));
    
    editor->ctx = ctx;
    editor->event = EDITOR_EVENT_NONE;
    editor->count = 0;
    editor->cursor = 0;
    editor->head = 0;
    editor->position = -1;

    return 0;
}

int editor_process_key(editor_t *editor)
{
    char c;
    
    if (read(STDIN_FILENO, &c, 1) < 1) {
        return 1;
    }
    
    editor->event = EDITOR_EVENT_NONE;

    switch(c) {
    case 3:  /* CTRL-C */
        editor_clear(editor);
        editor->event = EDITOR_EVENT_TERMINATE;
        break;

    case 21:  /* CTRL-U */
        editor_delete_line(editor);
        break;

    case 127:  /* DELETE */
        editor_backspace(editor);
        break;

    case 13:  /* ENTER */
        if (editor_enter(editor) > 0) {
            editor->event = EDITOR_EVENT_SEND;
        }
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
    int cols = terminal_columns(STDIN_FILENO);
    int size = strlen(editor->ctx->selected) + 1;
    int start = editor->cursor - (cols - size - 1) < 0 ?
        0 : editor->cursor - (cols - size - 1);

    printf("\r\x1b[7;34m%s:\x1b[0m\x1b[7m%.*s\x1b[0m \x1b[0K",
        editor->ctx->selected,
        cols - size - 1, editor->scratch + start);

    printf("\r\x1b[%dC", editor->cursor + size);

    fflush(stdout);

    return 0;
}