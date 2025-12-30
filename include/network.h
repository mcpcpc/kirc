/*
 * network.h
 * Header for the network module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_NETWORK_H
#define __KIRC_NETWORK_H

#include "kirc.h"
#include "transport.h"
#include "ansi.h"
#include "helper.h"
#include "output.h"

struct network {
    struct kirc_context *ctx;
    struct transport *transport;
    char buffer[MESSAGE_MAX_LEN];
    int len;
};

int network_send(struct network *network, const char *fmt, ...);
int network_receive(struct network *network);
int network_connect(struct network *network);
int network_command_handler(struct network *network, char *msg, struct output *output);
int network_send_credentials(struct network *network);

int network_init(struct network *network,
        struct transport *transport, struct kirc_context *ctx);
int network_free(struct network *network);

#endif  // __KIRC_NETWORK_H
