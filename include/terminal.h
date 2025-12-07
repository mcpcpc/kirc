#ifndef __KIRC_TERMINAL_H
#define __KIRC_TERMINAL_H

#include "kirc.h"

int terminal_columns(kirc_t *ctx);
int terminal_enable_raw(kirc_t *ctx);
void terminal_disable_raw(kirc_t *ctx);

#endif  // __KIRC_TERMINAL_H
