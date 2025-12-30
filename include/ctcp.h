/*
 * ctcp.h
 * Header for the CTCP module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_CTCP_H
#define __KIRC_CTCP_H

#include "kirc.h"
#include "event.h"
#include "network.h"

void ctcp_handle_clientinfo(struct network *network, struct event *event);
void ctcp_handle_ping(struct network *network, struct event *event);
void ctcp_handle_time(struct network *network, struct event *event);
void ctcp_handle_version(struct network *network, struct event *event);

#endif  // __KIRC_CTCP_H
