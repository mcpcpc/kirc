/*
 * ctcp.h
 * Header for the CTCP module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_CTCP_H
#define __KIRC_CTCP_H

#include "kirc.h"
#include "protocol.h"
#include "network.h"

typedef struct {
    protocol_event_t event;
    void (*handler)(network_t *network, protocol_t *protocol);
} ctcp_dispatch_entry_t;

/* Handle all CTCP events based on protocol event type */
void ctcp_handle_event(network_t *network, protocol_t *protocol);

#endif  // __KIRC_CTCP_H
