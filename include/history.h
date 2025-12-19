#ifndef __KIRC_HISTORY_H
#define __KIRC_HISTORY_H

#include "kirc.h"

typedef struct {
    char buffer[KIRC_HISTORY_SIZE][RFC1459_MESSAGE_MAX_LEN];
    int head;
    int count;
    int position;
} history_t;

int history_init(history_t *history);
int history_add(history_t *history, const char *entry);
int history_previous(history_t *history, char *scratch);
int history_next(history_t *history, char *scratch);

char *history_get_last(history_t *history);

#endif  // __KIRC_HISTORY_H
