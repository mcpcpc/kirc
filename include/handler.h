/*
 * handler.h
 * Header for handler dispatch module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_HANDLER_H
#define __KIRC_HANDLER_H

#include "kirc.h"
#include "event.h"
#include "network.h"

typedef void (*event_handler_fn)(struct network *network, struct event *event);

struct handler_entry {
    enum event_type type;
    event_handler_fn handler;
};

struct handler {
    struct kirc_context *ctx;
    struct handler_entry entries[KIRC_HANDLER_MAX_ENTRIES];
    event_handler_fn default_handler;
    int count;
};

/* Set the default handler for unhandled events */
void handler_set_default(struct handler *handler, event_handler_fn handler_fn);

/* Register a handler for a specific event type */
void handler_register(struct handler *handler, enum event_type type, event_handler_fn handler_fn);

/* Dispatch an event to the appropriate handler(s) */
void handler_dispatch(struct handler *handler, struct network *network, struct event *event);

/* Initialize the handler system - registers all built-in handlers */
int handler_init(struct handler *handler, struct kirc_context *ctx);

#endif  // __KIRC_HANDLER_H
