#ifndef __KIRC_RENDER_H
#define __KIRC_RENDER_H

#include "kirc.h"
#include "event.h"

typedef struct {
    char default_channel[512];
    int verbose;
} render_t;

void render_event(render_t *r, const event_t *ev,
        int terminal_cols);
void render_editor_line(render_t *r, const editor_t *e);

#endif  // __KIRC_RENDER_H
