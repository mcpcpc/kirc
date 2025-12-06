#ifndef __KIRC_EDITOR_H
#define __KIRC_EDITOR_H

#include "kirc.h"

typedef struct {
    kirc_context_t ctx;
    char *prompt;
    char *buf;
    size_t buflen;
    size_t plenb;
    size_t plenu8;
    size_t posb;
    size_t posu8;
    size_t oldposb;
    size_t oldposu8;
    size_t lenb;
    size_t lenu8;
    size_t cols;
    int history_index;
} editor_context_t;

void editor_init(editor_context_t *ectx, kirc_context_t *ctx,
        char *buffer, size_t buflen, char *prompt);
void editor_reset(editor_context_t *ectx);
void editor_refresh(editor_context_t *ectx);

int editor_process_keypress(editor_context_t *ectx);



#endif  // __KIRC_EDITOR_H
