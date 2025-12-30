/*
 * handler.c
 * Unified event handler dispatch system
 * Author: Michael Czigler
 * License: MIT
 */

#include "handler.h"

void handler_set_default(struct handler *handler,
        event_handler_fn handler_fn)
{
    handler->default_handler = handler_fn;
}  

void handler_register(struct handler *handler, enum event_type type,
        event_handler_fn handler_fn)
{
    if (handler->count >= KIRC_HANDLER_MAX_ENTRIES) {
        return;
    }
    
    handler->entries[handler->count].type = type;
    handler->entries[handler->count].handler = handler_fn;
    handler->count++;
}

int handler_init(struct handler *handler, struct kirc_context *ctx)
{
    memset(handler, 0, sizeof(*handler));

    handler->ctx = ctx;
    handler->count = 0;

    return 0;
}

void handler_dispatch(struct handler *handler, struct network *network,
        struct event *event)
{
    if (handler == NULL || network == NULL || event == NULL) {
        return;
    }
    
    for (int i = 0; i < handler->count; ++i) {
        if (handler->entries[i].type == event->type) {
            handler->entries[i].handler(network, event);
            return;
        }
    }

    /* If no handler found, display raw message */
    handler->default_handler(network, event);
}
