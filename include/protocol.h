/*
 * protocol.h
 * Header for the protocol module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_PROTOCOL_H
#define __KIRC_PROTOCOL_H

#include "kirc.h"
#include "ansi.h"
#include "event.h"
#include "helper.h"
#include "network.h"
#include "output.h"

void protocol_noop(struct network *network, struct event *event, struct output *output);
void protocol_ping(struct network *network, struct event *event, struct output *output);
void protocol_authenticate(struct network *network, struct event *event, struct output *output);
void protocol_welcome(struct network *network, struct event *event, struct output *output);
void protocol_raw(struct network *network, struct event *event, struct output *output);
void protocol_info(struct network *network, struct event *event, struct output *output);
void protocol_error(struct network *network, struct event *event, struct output *output);
void protocol_notice(struct network *network, struct event *event, struct output *output);
void protocol_privmsg(struct network *network, struct event *event, struct output *output);
void protocol_nick(struct network *network, struct event *event, struct output *output);
void protocol_part(struct network *network, struct event *event, struct output *output);
void protocol_ctcp_action(struct network *network, struct event *event, struct output *output);
void protocol_ctcp_info(struct network *network, struct event *event, struct output *output);

#endif  // __KIRC_PROTOCOL_H
