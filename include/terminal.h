#ifndef __KIRC_TERMINAL_H
#define __KIRC_TERMINAL_H

#include "kirc.h"

typedef struct {
    kirc_t ctx;
    int raw_mode_enabled;
    struct termios original;
} terminal_t;

int terminal_columns(kirc_t *ctx);

int terminal_init(terminal_t *terminal, kirc_t *ctx);
int terminal_enable_raw(terminal_t *terminal);
void terminal_disable_raw(terminal_t *terminal);

#endif  // __KIRC_TERMINAL_H
