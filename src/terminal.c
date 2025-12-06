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

int terminal_open(terminal_t *t)
{
    if (!t) {
        errno = EINVAL;
        return -1;
    }

    if (t->tty_fd > 0) {
        return 0;  /* already open */
    }

    t->tty_fd = open("/dev/tty", O_RDONLY | O_CLOEXEC);
    if (t->tty_fd == -1) {
        perror("open /dev/tty");
        return -1;
    }

    t->raw_mode_enabled   = 0;
    t->atexit_registered  = 0;

    return 0;
}

int terminal_enable_raw(terminal_t *t)
{
    if (!t || t->tty_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    if (!isatty(t->tty_fd)) {
        errno = ENOTTY;
        return -1;
    }

    if (!t->raw_mode_enabled) {
        if (tcgetattr(t->tty_fd, &t->original_termios) == -1) {
            return -1;
        }

        struct termios raw = t->original_termios;

        raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        raw.c_oflag &= ~(OPOST);
        raw.c_cflag |= (CS8);
        raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        raw.c_cc[VMIN]  = 1;
        raw.c_cc[VTIME] = 0;

        if (tcsetattr(t->tty_fd, TCSAFLUSH, &raw) == -1) {
            return -1;
        }

        t->raw_mode_enabled = 1;
    }

    return 0;
}

void terminal_disable_raw(terminal_t *t)
{
    if (!t || !t->raw_mode_enabled) {
        return;
    }

    tcsetattr(t->tty_fd, TCSAFLUSH, &t->original_termios);
    t->raw_mode_enabled = 0;
}

int terminal_columns(terminal_t *t)
{
    if (!t || t->tty_fd < 0) {
        return 80;
    }

    struct winsize ws;
    if (ioctl(t->tty_fd, TIOCGWINSZ, &ws) != -1 && ws.ws_col > 0) {
        return ws.ws_col;
    }

    /* Fallback: manual probing */
    int start = terminal_get_cursor_column(t->tty_fd, STDOUT_FILENO);
    if (start == -1) {
        return 80;
    }

    /* Move far right */
    if (write(STDOUT_FILENO, "\x1b[999C", 6) != 6) {
        return 80;
    }

    int end = terminal_get_cursor_column(t->tty_fd, STDOUT_FILENO);
    if (end == -1) {
        return 80;
    }

    /* Move cursor back */
    if (end > start) {
        char seq[32];
        int diff = end - start;
        snprintf(seq, sizeof(seq), "\x1b[%dD", diff);
        write(STDOUT_FILENO, seq, strnlen(seq, sizeof(seq)));
    }

    return (end > 0) ? end : 80;
}
