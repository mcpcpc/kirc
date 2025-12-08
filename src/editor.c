#include "editor.h"

static void editor_backspace(kirc_t *ctx)
{
}

static void editor_enter(kirc_t *ctx)
{
}

static void editor_delete(kirc_t *ctx)
{
}

static void editor_history(kirc_t *ctx, int dir)
{
}

static void editor_move_right(kirc_t *ctx)
{
}

static void editor_move_left(kirc_t *ctx)
{
}

static void editor_move_home(kirc_t *ctx)
{
}

static void editor_move_end(kirc_t *ctx)
{
}

static void editor_escape(kirc_t *ctx)
{
    char seq[3];

    if (read(ctx->tty_fd, &seq[0], 1) != 1) {
        return;
    }

    if (read(ctx->tty_fd, &seq[1], 1) != 1) {
        return;
    }

    if (seq[0] == '[') {
        if (seq[1] >= '0' && seq[1] <= '9') {
            if (read(ctx->tty_fd, &seq[2], 1) != 1)
                return;
            if (seq[1] == '3' && seq[2] == '~') {
                editor_delete(ctx);
            }
        } else {
            switch (seq[1]) {
            case 'A':
                editor_history(ctx, 1);
                break;

            case 'B':
                editor_history(ctx, -1);
                break;

            case 'C':
                editor_move_right(ctx);
                break;

            case 'D':
                editor_move_left(ctx);
                break;

            case 'H':
                editor_move_home(ctx);
                break;

            case 'F':
                editor_move_end(ctx);
                break;
            }
        }
    }
}


int editor_process_key(kirc_t *ctx)
{
    char c;
    
    if (read(ctx->tty_fd, &c, 1) < 1) {
        return 1; 
    }

    switch(c) {
    case 3:  /* CTRL-C */
        errno = EAGAIN;
        return -1;

    case 127:  /* ESCAPE */
    case 8:  /* DEL */ 
        editor_backspace(ctx);
        break;

    case 13:  /* ENTER */
        editor_enter(ctx);
        break;

    case 27:  /* ESCAPE */
        editor_escape(ctx);
        break;

    default:
        break;
    }

    return 0;
}

int editor_render(kirc_t *ctx)
{
    int cols = terminal_columns(ctx);
}
