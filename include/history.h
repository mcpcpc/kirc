#ifndef __KIRC_HISTORY_H
#define __KIRC_HISTORY_H

#include "kirc.h"

typedef struct history_context_t {
    kirc_context_t ctx;
    char **items;
    int length;
    int max_length;
} history_context_t;

int history_init(history_context_t *hctx, int max_len);
void history_free(history_context_t *hctx);

int history_add(history_context_t *hctx, const char *line);
const char *history_get(history_context_t *hctx, int index);

#endif  // __KIRC_HISTORY_H
