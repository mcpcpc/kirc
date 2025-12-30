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

enum editor_state {
    EDITOR_STATE_NONE = 0,
    EDITOR_STATE_SEND,
    EDITOR_STATE_TERMINATE
};

struct editor {
    struct kirc_context *ctx;
    enum editor_state state;
    char scratch[MESSAGE_MAX_LEN];
    char history[KIRC_HISTORY_SIZE][MESSAGE_MAX_LEN];
    int head;
    int count;
    int position; /* -1 when not browsing history, otherwise index into history[] */
    int cursor;
};

char *editor_last_entry(struct editor *editor);
int editor_init(struct editor *editor, struct kirc_context *ctx);
int editor_process_key(struct editor *editor);
int editor_handle(struct editor *editor);

#endif  // __KIRC_EDITOR_H
