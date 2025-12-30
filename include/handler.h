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
#include "protocol.h"
#include "event.h"
#include "ctcp.h"

typedef void (*event_handler_fn)(struct network *network, struct event *event);

struct handler_entry_table {
    enum event_type type;
    event_handler_fn handler;
};

/* Dispatch an event to the appropriate handler(s) */
void handler_dispatch(struct network *network, struct event *event);

#endif  // __KIRC_HANDLER_H
