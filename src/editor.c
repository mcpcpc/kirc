/*
 * editor.c
 * Text editor functionality for the IRC client
 * Author: Michael Czigler
 * License: MIT
 */

#include "editor.h"

/**
 * editor_backspace() - Delete the character before the cursor
 * @editor: Editor state structure
 *
 * Removes the UTF-8 character immediately before the cursor position,
 * properly handling multi-byte UTF-8 sequences. Does nothing if cursor
 * is at the beginning of the line.
 */
static void editor_backspace(struct editor *editor)
{
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    if (editor->cursor <= 0) {
        return;  /* nothing to delete or out of range */
    }

    /* Get length of previous UTF-8 character */
    int bytes = utf8_prev_char_len(editor->scratch, editor->cursor);
    
    if (bytes == 0) {
        return;
    }

    editor->cursor -= bytes;

    memmove(editor->scratch + editor->cursor,
        editor->scratch + editor->cursor + bytes,
        len - (editor->cursor + bytes) + 1);
}

/**
 * editor_delete_line() - Clear the entire input line
 * @editor: Editor state structure
 *
 * Erases all text from the scratch buffer and resets the cursor to the
 * beginning of the line.
 */
static void editor_delete_line(struct editor *editor)
{
    editor->scratch[0] = '\0';
    editor->cursor = 0;
}

/**
 * editor_enter() - Process enter key to submit current line
 * @editor: Editor state structure
 *
 * Saves the current line to history, advances the history pointer, and
 * clears the scratch buffer. Does nothing if the line is empty.
 *
 * Return: 1 if line was submitted, 0 if line was empty
 */
static int editor_enter(struct editor *editor)
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

/**
 * editor_delete() - Delete the character at the cursor
 * @editor: Editor state structure
 *
 * Removes the UTF-8 character at the current cursor position, properly
 * handling multi-byte UTF-8 sequences. Does nothing if cursor is at the
 * end of the line.
 */
static void editor_delete(struct editor *editor)
{
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    if (editor->cursor >= len) {
        return;  /* at end of scratch string */
    }

    /* Get length of next UTF-8 character using mbrtowc */
    int bytes = utf8_next_char_len(editor->scratch, editor->cursor, len);
    
    if (bytes == 0) {
        return;
    }

    memmove(editor->scratch + editor->cursor,
        editor->scratch + editor->cursor + bytes,
        len - (editor->cursor + bytes) + 1);
}

/**
 * editor_history() - Navigate through command history
 * @editor: Editor state structure
 * @dir: Direction to navigate (positive for previous, negative for next)
 *
 * Allows browsing through the command history using up/down arrows. Loads
 * the previous or next history entry into the scratch buffer and updates
 * the cursor position. Handles wraparound at history boundaries.
 */
static void editor_history(struct editor *editor, int dir)
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

/**
 * editor_move_right() - Move cursor one character to the right
 * @editor: Editor state structure
 *
 * Advances the cursor by one UTF-8 character, properly handling multi-byte
 * sequences. Does nothing if cursor is at the end of the line.
 */
static void editor_move_right(struct editor *editor)
{
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    if (editor->cursor >= len) {
        return; /* at end */
    }

    /* Use mbrtowc to get proper character length */
    int adv = utf8_next_char_len(editor->scratch, editor->cursor, len);
    
    if (adv > 0 && editor->cursor + adv <= len) {
        editor->cursor += adv;
    } else {
        /* Invalid sequence or truncated, move to end */
        editor->cursor = len;
    }
}

/**
 * editor_move_left() - Move cursor one character to the left
 * @editor: Editor state structure
 *
 * Moves the cursor back by one UTF-8 character, properly handling multi-byte
 * sequences. Does nothing if cursor is at the beginning of the line.
 */
static void editor_move_left(struct editor *editor)
{
    if (editor->cursor <= 0) {
        return;
    }

    /* Get length of previous UTF-8 character */
    int bytes = utf8_prev_char_len(editor->scratch, editor->cursor);
    
    if (bytes > 0) {
        editor->cursor -= bytes;
    }
}

/**
 * editor_move_home() - Move cursor to beginning of line
 * @editor: Editor state structure
 *
 * Sets the cursor position to the start of the input line.
 */
static void editor_move_home(struct editor *editor)
{
    editor->cursor = 0;
}

/**
 * editor_move_end() - Move cursor to end of line
 * @editor: Editor state structure
 *
 * Sets the cursor position to the end of the current input line.
 */
static void editor_move_end(struct editor *editor)
{
    size_t siz = sizeof(editor->scratch) - 1;

    editor->cursor = strnlen(editor->scratch, siz);
}

/**
 * editor_escape() - Handle escape sequences for special keys
 * @editor: Editor state structure
 *
 * Processes ANSI escape sequences for cursor movement and special keys
 * (arrow keys, Home, End, Delete). Reads additional bytes from stdin
 * to determine the complete escape sequence and executes the appropriate
 * editor action.
 */
static void editor_escape(struct editor *editor)
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

/**
 * editor_insert() - Insert a single character at cursor position
 * @editor: Editor state structure
 * @c: Character to insert
 *
 * Inserts a single byte character at the current cursor position, shifting
 * existing text to the right. Advances the cursor after insertion. Does
 * nothing if the buffer is full.
 */
static void editor_insert(struct editor *editor, char c)
{
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    if (editor->cursor > siz) {
        return;  /* at end of scratch */
    }

    if (len + 1 >= siz) {
        return;  /* scratch full */
    }

    memmove(editor->scratch + editor->cursor + 1,
        editor->scratch + editor->cursor,
        len - editor->cursor + 1);

    editor->scratch[editor->cursor] = c;
    editor->cursor++;
}

/**
 * editor_insert_bytes() - Insert multiple bytes at cursor position
 * @editor: Editor state structure
 * @buf: Buffer containing bytes to insert
 * @n: Number of bytes to insert
 *
 * Inserts a sequence of bytes (typically a multi-byte UTF-8 character) at
 * the current cursor position. Validates UTF-8 encoding before insertion.
 * Shifts existing text to the right and advances the cursor.
 */
static void editor_insert_bytes(struct editor *editor, const char *buf, int n)
{
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    if (editor->cursor > siz) {
        return;  /* at end of scratch */
    }

    if (len + n >= siz) {
        return;  /* scratch full */
    }

    /* Validate UTF-8 sequence before inserting */
    if (!utf8_validate(buf, n)) {
        return;  /* invalid UTF-8, reject */
    }

    memmove(editor->scratch + editor->cursor + n,
        editor->scratch + editor->cursor,
        len - editor->cursor + 1);

    memcpy(editor->scratch + editor->cursor, buf, n);
    editor->cursor += n;
}

/**
 * editor_clear() - Clear the current line on terminal
 * @editor: Editor state structure (unused)
 *
 * Sends ANSI escape codes to clear the current terminal line, moving
 * the cursor to the beginning.
 */
static void editor_clear(struct editor *editor)
{
    printf("\r" CLEAR_LINE);
}

/**
 * display_width_bytes() - Calculate display width of UTF-8 string
 * @s: UTF-8 encoded string
 * @bytes: Number of bytes to measure
 *
 * Computes the display width (number of terminal columns) occupied by
 * a UTF-8 string, accounting for wide characters (e.g., CJK characters).
 * Handles invalid sequences by treating them as width 1.
 *
 * Return: Display width in terminal columns
 */
static int display_width_bytes(const char *s, int bytes)
{
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    int pos = 0;
    int wsum = 0;

    while (pos < bytes) {
        wchar_t wc;
        size_t ret = mbrtowc(&wc, s + pos, bytes - pos, &st);
        if (ret == (size_t)-1 || ret == (size_t)-2) {
            /* invalid sequence: treat as width 1 */
            pos++;
            wsum += 1;
            memset(&st, 0, sizeof(st));
            continue;
        }
        if (ret == 0) {
            pos++;
            continue;
        }
        int w = wcwidth(wc);
        if (w < 0) w = 0;
        wsum += w;
        pos += ret;
    }

    return wsum;
}

/**
 * editor_tab() - Insert tab as spaces
 * @editor: Editor state structure
 *
 * Inserts KIRC_TAB_WIDTH spaces at the cursor position to simulate
 * a tab character.
 */
static void editor_tab(struct editor *editor)
{
    int width = KIRC_TAB_WIDTH;

    for (int i = 0; i < width; ++i) {
        editor_insert(editor, ' ');
    } 
}

/**
 * editor_last_entry() - Get the most recent history entry
 * @editor: Editor state structure
 *
 * Returns a pointer to the last command entered in the history buffer.
 *
 * Return: Pointer to the most recent history string
 */
char *editor_last_entry(struct editor *editor)
{
    int head = editor->head;
    int last = (head - 1 + KIRC_HISTORY_SIZE) % KIRC_HISTORY_SIZE;

    return editor->history[last];
}

/**
 * editor_init() - Initialize the editor state
 * @editor: Editor structure to initialize
 * @ctx: IRC context structure
 *
 * Initializes the editor with zeroed state, sets up locale for UTF-8
 * support, and associates it with the IRC context. Prepares the editor
 * for input processing.
 *
 * Return: 0 on success, -1 if editor or ctx is NULL
 */
int editor_init(struct editor *editor, struct kirc_context *ctx)
{
    if ((editor == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(editor, 0, sizeof(*editor));
    
    editor->ctx = ctx;
    editor->state = EDITOR_STATE_NONE;
    editor->position = -1;

    setlocale(LC_CTYPE, "");

    return 0;
}

/**
 * editor_process_key() - Process a single keystroke
 * @editor: Editor state structure
 *
 * Reads and processes one character from stdin. Handles control characters,
 * escape sequences, and UTF-8 multi-byte input. Updates editor state
 * (SEND, TERMINATE, or NONE) based on the key pressed.
 *
 * Return: 0 on success, 1 if read failed
 */
int editor_process_key(struct editor *editor)
{
    char c;
    
    if (read(STDIN_FILENO, &c, 1) < 1) {
        return 1;
    }
    
    editor->state = EDITOR_STATE_NONE;

    switch(c) {
    case HT:  /* CTRL-I or TAB */
        editor_tab(editor);
        break;

    case ETX:  /* CTRL-C */
        editor_clear(editor);
        editor->state = EDITOR_STATE_TERMINATE;
        break;

    case NAK:  /* CTRL-U */
        editor_delete_line(editor);
        break;

    case BS:  /* CTRL-H */
    case DEL:
        editor_backspace(editor);
        break;

    case CR:  /* CTRL-M or ENTER */
        if (editor_enter(editor) > 0) {
            editor->state = EDITOR_STATE_SEND;
        }
        break;

    case ESC:  /* CTRL-[ */
        editor_escape(editor);
        break;

    case SOH:  /* CTRL-A */
    case STX:  /* CTRL-B */
    case EOT:  /* CTRL-D */
    case ENQ:  /* CTRL-E */
    case ACK:  /* CTRL-F */
    case BEL:  /* CTRL-G */
    case LF:  /* CTRL-J */
    case VT:  /* CTRL-K */
    case FF:  /* CTRL-L */
    case SO:  /* CTRL-N */
    case SI:  /* CTRL-O */
    case DLE:  /* CTRL-P */
    case DC1:  /* CTRL-Q */
    case DC2:  /* CTRL-R */
    case DC3:  /* CTRL-S */
    case DC4:  /* CTRL-T */
    case SYN:  /* CTRL-V */
    case ETB:  /* CTRL-W */
    case CAN:  /* CTRL-X */
    case EM:  /* CTRL-Y */
    case SUB:  /* CTRL-Z */
    case FS:  /* CTRL-\ */
    case GS:  /* CTRL-] */
    case RS:  /* CTRL-^ */
    case US:  /* CTRL-_ */
        break;  /* not implemented yet */

    default:
        /* handle UTF-8 multi-byte input: read remaining continuation bytes */
        {
            unsigned char uc = (unsigned char)c;
            char buf[5];
            int need = 0;
            buf[0] = c;

            if ((uc & 0x80) == 0) {
                editor_insert(editor, c);
            } else {
                if ((uc & 0xE0) == 0xC0) need = 1;
                else if ((uc & 0xF0) == 0xE0) need = 2;
                else if ((uc & 0xF8) == 0xF0) need = 3;
                else need = 0;

                int i;
                for (i = 1; i <= need; i++) {
                    if (read(STDIN_FILENO, &buf[i], 1) != 1) {
                        break;
                    }
                }

                editor_insert_bytes(editor, buf, 1 + (i - 1));
            }
        }
        break;
    }

    return 0;
}

/**
 * editor_handle() - Render the editor display
 * @editor: Editor state structure
 *
 * Renders the current editor state to the terminal, displaying the target
 * channel/user and the input text. Handles line scrolling when text exceeds
 * terminal width and positions the cursor correctly. Accounts for UTF-8
 * character widths for proper display alignment.
 *
 * Return: 0 on success
 */
int editor_handle(struct editor *editor)
{
    int cols = terminal_columns(STDIN_FILENO);
    int size = strlen(editor->ctx->target) + 1;
    int avail = cols - size - 1;
    int siz = sizeof(editor->scratch) - 1;
    int len = strnlen(editor->scratch, siz);

    /* compute display width of bytes up to cursor */
    int cursor_disp = display_width_bytes(editor->scratch, editor->cursor);

    /* choose start byte offset so that the substring ending at cursor fits in avail */
    int start = editor->cursor;
    int used = 0;
    while (start > 0) {
        int char_bytes = utf8_prev_char_len(editor->scratch, start);
        if (char_bytes == 0) break;
        int cw = display_width_bytes(editor->scratch + start - char_bytes, char_bytes);
        if (used + cw > avail) break;
        used += cw;
        start -= char_bytes;
    }

    /* compute how many bytes we can print from start given avail */
    int bytes_to_print = 0;
    int p = start;
    int printed_width = 0;
    while (p < len) {
        int cb = utf8_next_char_len(editor->scratch, p, len);
        if (cb == 0) break;
        int cw = display_width_bytes(editor->scratch + p, cb);
        if (printed_width + cw > avail) break;
        printed_width += cw;
        p += cb;
    }
    bytes_to_print = p - start;

    printf("\r%s:", editor->ctx->target);

    if (bytes_to_print > 0) {
        fwrite(editor->scratch + start, 1, bytes_to_print, stdout);
    }

    printf(" " CLEAR_LINE);

    printf("\r\x1b[%dC", cursor_disp + size);

    fflush(stdout);

    return 0;
}