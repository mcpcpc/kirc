#ifndef __KIRC_EDITOR_H
#define __KIRC_EDITOR_H

#include "kirc.h"
#include "terminal.h"

typedef struct {
    kirc_t *ctx;
    char scratch[KIRC_SCRATCH_MAX][RFC1459_MESSAGE_MAX_LEN];
    int scratch_max;
    int scratch_size;
    int scratch_current;
} editor_t;

int editor_init(editor_t *editor, kirc_t *ctx);
int editor_process_key(editor_t *editor);
int editor_render(editor_t *editor);

#endif  // __KIRC_EDITOR_H
