#ifndef __KIRC_EDITOR_H
#define __KIRC_EDITOR_H

#include "kirc.h"
#include "terminal.h"

typedef struct {
    kirc_t *ctx;
    char scratch[RFC1459_MESSAGE_MAX_LEN];
    char history[KIRC_HISTORY_SIZE][RFC1459_MESSAGE_MAX_LEN];
    int head;
    int count;
    int position; /* -1 when not browsing history, otherwise index into history[] */
    int cursor;
} editor_t;

int editor_init(editor_t *editor, kirc_t *ctx);
int editor_process_key(editor_t *editor);
int editor_render(editor_t *editor);

#endif  // __KIRC_EDITOR_H
