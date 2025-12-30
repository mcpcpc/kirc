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
#include "output.h"

typedef void (*event_handler_fn)(struct network *network, struct event *event, struct output *output);

struct handler {
    struct kirc_context *ctx;
    event_handler_fn handlers[KIRC_EVENT_TYPE_MAX];
    event_handler_fn default_handler;
};

void handler_set_default(struct handler *handler, event_handler_fn handler_fn);
void handler_register(struct handler *handler, enum event_type type, event_handler_fn handler_fn);
void handler_dispatch(struct handler *handler, struct network *network, struct event *event, struct output *output);

int handler_init(struct handler *handler, struct kirc_context *ctx);

#endif  // __KIRC_HANDLER_H
