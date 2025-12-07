#include "editor.h"

static void editor_backspace(kirc_t *ctx)
{
}

static void editor_enter(kirc_t *ctx)
{
}

static void editor_escape(kirc_t *ctx)
{
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
