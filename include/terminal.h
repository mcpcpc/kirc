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

struct terminal {
    struct kirc_context *ctx;
    struct termios original;
    int raw_mode_enabled;
};

int terminal_columns(int tty_fd);

int terminal_init(struct terminal *terminal,
        struct kirc_context *ctx);
int terminal_enable_raw(struct terminal *terminal);
void terminal_disable_raw(struct terminal *terminal);

#endif  // __KIRC_TERMINAL_H
