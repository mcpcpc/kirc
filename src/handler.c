/*
 * handler.c
 * Unified event handler dispatch system
 * Author: Michael Czigler
 * License: MIT
 */

#include "handler.h"

void handler_default(struct handler *handler,
        event_handler_fn handler_fn)
{
    handler->default_handler = handler_fn;
}  

void handler_register(struct handler *handler, enum event_type type,
        event_handler_fn handler_fn)
{
    if (type < 0 || type >= KIRC_EVENT_TYPE_MAX) {
        return;
    }
    
    handler->handlers[type] = handler_fn;
}

int handler_init(struct handler *handler, struct kirc_context *ctx)
{
    memset(handler, 0, sizeof(*handler));

    handler->ctx = ctx;

    return 0;
}

void handler_dispatch(struct handler *handler, struct network *network,
        struct event *event, struct output *output)
{
    if (handler == NULL || network == NULL || event == NULL) {
        return;
    }
    
    /* O(1) direct array lookup instead of O(n) linear search */
    if (event->type >= 0 && event->type < KIRC_EVENT_TYPE_MAX) {
        event_handler_fn fn = handler->handlers[event->type];
        if (fn != NULL) {
            fn(network, event, output);
            return;
        }
    }

    /* If no handler found, display raw message */
    if (handler->default_handler != NULL) {
        handler->default_handler(network, event, output);
    }
}
