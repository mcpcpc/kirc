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

typedef struct {
    kirc_t *ctx;
    transport_t *transport;
    char buffer[MESSAGE_MAX_LEN];
    int len;
} network_t;

int network_send(network_t *network, const char *fmt, ...);
int network_receive(network_t *network);
int network_connect(network_t *network);
int network_command_handler(network_t *network, char *msg);
int network_authenticate(network_t *network);
int network_join_channels(network_t *network);

int network_init(network_t *network,
        transport_t *transport, kirc_t *ctx);
int network_free(network_t *network);

#endif  // __KIRC_NETWORK_H
