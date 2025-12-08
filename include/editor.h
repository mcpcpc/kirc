#ifndef __KIRC_EDITOR_H
#define __KIRC_EDITOR_H

#include "kirc.h"
#include "terminal.h"

int editor_process_key(kirc_t *ctx);
int editor_render(kirc_t *ctx);

#endif  // __KIRC_EDITOR_H
