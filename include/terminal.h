#ifndef __KIRC_TERMINAL_H
#define __KIRC_TERMINAL_H

#include "kirc.h"

typedef struct {
    int tty_fd;
    struct termios original;
    int raw_enabled;
    int atexit_registered;
} terminal_context_t;

int terminal_open(terminal_context_t *tctx);
int terminal_enable_raw_mode(terminal_context_t *tctx);
void terminal_disable_raw_mode(terminal_context_t *tctx);

int terminal_get_columns(terminal_context_t *tctx,
    int output_fd);
int terminal_get_cursor_position(terminal_context_t *tctx,
    int output_fd);

#endif  // __KIRC_TERMINAL_H
