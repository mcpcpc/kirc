/*
 * terminal.c
 * Terminal interface and display management
 * Author: Michael Czigler
 * License: MIT
 */

#include "terminal.h"

static int terminal_get_cursor_column(int in_fd, int out_fd)
{
    char buf[32];
    unsigned int i = 0;
    int row = 0, col = 0;

    /* Request cursor position */
    if (write(out_fd, "\x1b[6n", 4) != 4) {
        return -1;
    }

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

    if (sscanf(buf + 2, "%d;%d", &row, &col) != 2) {
        return -1;
    }

    return col;
}

int terminal_columns(int tty_fd)
{
    struct winsize ws;

    if (ioctl(tty_fd, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0) {
        return ws.ws_col;
    }

    /* Fallback: manual probing */
    int start = terminal_get_cursor_column(tty_fd,
        STDOUT_FILENO);

    if (start == -1) {
        return KIRC_DEFAULT_COLUMNS;
    }

    /* Move far right */
    if (write(STDOUT_FILENO, "\x1b[999C", 6) != 6) {
        return KIRC_DEFAULT_COLUMNS;
    }

    int end = terminal_get_cursor_column(tty_fd,
        STDOUT_FILENO);

    if (end == -1) {
        return KIRC_DEFAULT_COLUMNS;
    }

    /* Move cursor back */
    if (end > start) {
        char seq[32];
        int diff = end - start;
        snprintf(seq, sizeof(seq), "\x1b[%dD", diff);

        if (write(STDOUT_FILENO, seq,
            strnlen(seq, sizeof(seq))) < 32) {
            return KIRC_DEFAULT_COLUMNS;
        }
    }

    return (end > 0) ? end : KIRC_DEFAULT_COLUMNS;
}

int terminal_enable_raw(terminal_t *terminal)
{
    if (!isatty(STDIN_FILENO)) {
        return -1;
    }

    if (terminal->raw_mode_enabled) {
        return 0;
    }
    
    if (tcgetattr(STDIN_FILENO,
        &terminal->original) == -1) {
        return -1;
    }

    struct termios raw = terminal->original;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        return -1;
    }

    terminal->raw_mode_enabled = 1;

    return 0;
}

void terminal_disable_raw(terminal_t *terminal)
{
    if (!terminal->raw_mode_enabled) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal->original);
    terminal->raw_mode_enabled = 0;
}

int terminal_init(terminal_t *terminal, kirc_t *ctx)
{
    memset(terminal, 0, sizeof(*terminal));   

    terminal->ctx = ctx;

    return 0;
}
