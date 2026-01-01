/*
 * handler.c
 * Unified event handler dispatch system
 * Author: Michael Czigler
 * License: MIT
 */

#include "handler.h"

/**
 * handler_default() - Set the default event handler
 * @handler: Handler structure to configure
 * @handler_fn: Function to call for unhandled events
 *
 * Registers a fallback handler that will be called for events that don't
 * have a specific handler registered.
 */
void handler_default(struct handler *handler,
        event_handler_fn handler_fn)
{
    handler->default_handler = handler_fn;
}

/**
 * handler_register() - Register a handler for an event type
 * @handler: Handler structure to configure
 * @type: Event type to register handler for
 * @handler_fn: Function to call when this event occurs
 *
 * Associates a specific handler function with an IRC event type.
 * The handler will be invoked via O(1) array lookup when the event
 * is dispatched. Does nothing if the event type is out of range.
 */
void handler_register(struct handler *handler, enum event_type type,
        event_handler_fn handler_fn)
{
    if (type < 0 || type >= KIRC_EVENT_TYPE_MAX) {
        return;
    }
    
    handler->handlers[type] = handler_fn;
}

/**
 * handler_init() - Initialize the handler dispatch system
 * @handler: Handler structure to initialize
 * @ctx: IRC context structure
 *
 * Initializes the handler system by zeroing the structure and associating
 * it with an IRC context. All event handlers are initially NULL.
 *
 * Return: 0 on success, -1 if handler or ctx is NULL
 */
int handler_init(struct handler *handler, struct kirc_context *ctx)
{
    if ((handler == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(handler, 0, sizeof(*handler));

    handler->ctx = ctx;

    return 0;
}

/**
 * handler_dispatch() - Dispatch event to appropriate handler
 * @handler: Handler structure containing registered handlers
 * @network: Network connection for the event
 * @event: Event to dispatch
 * @output: Output buffer for handler responses
 *
 * Dispatches an IRC event to its registered handler using O(1) array lookup.
 * If no specific handler is registered, calls the default handler (typically
 * displays raw message). Does nothing if required parameters are NULL.
 */
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
