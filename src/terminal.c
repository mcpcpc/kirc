#include "terminal.h"

/* 
 * For atexit: we need a static pointer to the context
 * we should restore.
 */
 
static terminal_context_t *g_term_for_atexit = NULL;

static void terminal_atexit_disable(void)
{
    if (g_term_for_atexit) {
        /* Best-effort restore; ignore errors. */
        if (g_term_for_atexit->raw_enabled) {
            (void)tcsetattr(g_term_for_atexit->tty_fd,
                             TCSAFLUSH,
                             &g_term_for_atexit->original);
            g_term_for_atexit->raw_enabled = 0;
        }
    }
}

int terminal_open(terminal_context_t *tctx)
{
    if (!tctx) {
        errno = EINVAL;
        return -1;
    }

    int fd = open("/dev/tty", O_RDONLY);
    if (fd == -1) {
        /* Same spirit as your original open_tty: caller likely exits. */
        perror("failed to open /dev/tty");
        return -1;
    }

    tctx->tty_fd = fd;
    tctx->raw_enabled = 0;
    tctx->atexit_registered = 0;

    return 0;
}

int terminal_enable_raw_mode(terminal_context_t *tctx)
{
    struct termios raw;

    if (!tctx) {
        errno = EINVAL;
        return -1;
    }

    if (!isatty(tctx->tty_fd)) {
        errno = ENOTTY;
        return -1;
    }

    if (tcgetattr(tctx->tty_fd, &tctx->original) == -1) {
        return -1;
    }

    raw = tctx->original;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(tctx->tty_fd, TCSAFLUSH, &raw) < 0) {
        return -1;
    }

    tctx->raw_enabled = 1;

    if (!tctx->atexit_registered) {
        g_term_for_atexit = tctx;
        atexit(terminal_atexit_disable);
        tctx->atexit_registered = 1;
    }

    return 0;
}

void terminal_disable_raw_mode(terminal_context_t *tctx)
{
    if (!tctx || !tctx->raw_enabled) {
        return;
    }

    if (tcsetattr(tctx->tty_fd, TCSAFLUSH, &tctx->original) != -1) {
        tctx->raw_enabled = 0;
    }
}

int terminal_get_cursor_position(terminal_context_t *tctx,
        int output_fd)
{
    char buf[32];
    int rows, cols;
    unsigned int i = 0;

    int input_fd = term->tty_fd;

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

int terminal_get_columns(terminal_context_t *tctx,
        int output_fd)
{
    struct winsize ws;
    int cols;
    char seq[32];

    /* First try TIOCGWINSZ. */
    if (ioctl(output_fd, TIOCGWINSZ, &ws) != -1 && ws.ws_col != 0) {
        return ws.ws_col;
    }

    /* Fallback: use cursor position trick. */
    int input_fd = term->tty_fd;

    int start = terminal_get_cursor_position(term, output_fd);
    if (start == -1) {
        return 80;
    }

    /* Move cursor very far right. */
    if (write(output_fd, "\x1b[999C", 6) != 6) {
        return 80;
    }

    cols = terminal_get_cursor_position(tctx, output_fd);
    if (cols == -1) {
        return 80;
    }

    /* Move cursor back to original column. */
    if (cols > start) {
        int delta = cols - start;
        snprintf(seq, sizeof(seq), "\x1b[%dD", delta);
        (void)write(output_fd, seq, strlen(seq));
    }

    (void)input_fd; /* suppress potential unused warning if not used */

    return cols;
}
