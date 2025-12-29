/*
 * editor.h
 * Header for the editor module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_EDITOR_H
#define __KIRC_EDITOR_H

#include "kirc.h"
#include "terminal.h"
#include "ansi.h"
#include "utf8.h"

typedef enum {
    EDITOR_EVENT_NONE = 0,
    EDITOR_EVENT_SEND,
    EDITOR_EVENT_TERMINATE
} editor_event_t;

typedef struct {
    kirc_context_t *ctx;
    editor_event_t event;
    char scratch[MESSAGE_MAX_LEN];
    char history[KIRC_HISTORY_SIZE][MESSAGE_MAX_LEN];
    int head;
    int count;
    int position; /* -1 when not browsing history, otherwise index into history[] */
    int cursor;
} editor_t;

char *editor_last_entry(editor_t *editor);
int editor_init(editor_t *editor, kirc_context_t *ctx);
int editor_process_key(editor_t *editor);
int editor_handle(editor_t *editor);

#endif  // __KIRC_EDITOR_H
