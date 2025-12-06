#include "terminal.h"

int terminal_enable_raw(kirc_t *ctx)
{
    if (!isatty(ctx->tty_fd)) {
        return -1;
    }

    if (ctx->raw_mode_enabled) {
        return 0;
    }
    
    if (tcgetattr(ctx->tty_fd, &ctx->original) == -1) {
        return -1;
    }

    struct termios raw = ctx->original;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN]  = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(ctx->tty_fd, TCSAFLUSH, &raw) == -1) {
        return -1;
    }

    ctx->raw_mode_enabled = 1;

    return 0;
}

void terminal_disable_raw(kirc_t *ctx)
{
    if (!ctx->raw_mode_enabled) {
        return;
    }

    tcsetattr(ctx->tty_fd, TCSAFLUSH, &ctx->original);
    ctx->raw_mode_enabled = 0;
}
