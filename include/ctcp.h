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

struct ctcp_dispatch {
    enum protocol_event event;
    void (*handler)(struct network *network, struct protocol *protocol);
};

/* Handle all CTCP events based on protocol event type */
void ctcp_handle_event(struct network *network, struct protocol *protocol);

#endif  // __KIRC_CTCP_H
