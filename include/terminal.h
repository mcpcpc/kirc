/*
 * terminal.h
 * Header for the terminal module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_TERMINAL_H
#define __KIRC_TERMINAL_H

#include "kirc.h"
#include "ansi.h"

typedef struct {
    kirc_t *ctx;
    struct termios original;
    int raw_mode_enabled;
} terminal_t;

int terminal_columns(int tty_fd);

int terminal_init(terminal_t *terminal,
        kirc_context_t *ctx);
int terminal_enable_raw(terminal_t *terminal);
void terminal_disable_raw(terminal_t *terminal);

#endif  // __KIRC_TERMINAL_H
