#ifndef __KIRC_HISTORY_H
#define __KIRC_HISTORY_H

#include "kirc.h"

typedef struct {
    char **entries;
    int length;
    int capacity;
} history_t;

int history_init(history_t *h, int capacity);
void history_free(history_t *h);
int history_add(history_t *h, const char *line);
const char *history_get(history_t *h, int index);

#endif  // __KIRC_HISTORY_H
