#ifndef __KIRC_TERMINAL_H
#define __KIRC_TERMINAL_H

#include "kirc.h"

typedef struct {
    int tty_fd;
    struct termios original_termios;
    int raw_mode_enabled;
    int atexit_registered;
} terminal_t;

int terminal_open(terminal_t *t);
int terminal_enable_raw(terminal_t *t);
void terminal_disable_raw(terminal_t *t);
int terminal_columns(terminal_t *t);

#endif  // __KIRC_TERMINAL_H
