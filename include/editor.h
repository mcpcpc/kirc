#ifndef __KIRC_EDITOR_H
#define __KIRC_EDITOR_H

#include "kirc.h"
#include "history.h"
#include "utf8.h"
#include "terminal.h"

typedef struct {
    char  *prompt;
    char  *buffer;
    size_t buffer_size;

    size_t prompt_len_bytes;
    size_t prompt_len_u8;

    size_t cursor_byte;
    size_t cursor_u8;

    size_t prev_cursor_byte;
    size_t prev_cursor_u8;

    size_t content_len_bytes;
    size_t content_len_u8;

    size_t terminal_cols;

    int history_index;

    history_t *history;
    utf8_t *utf8;
    terminal_t *terminal;
} editor_t;

void editor_reset(editor_t *e);
int editor_process_key(editor_t *e);

#endif  // __KIRC_EDITOR_H
