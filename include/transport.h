/*
 * transport.h
 * Header for the transport module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_TRANSPORT_H
#define __KIRC_TRANSPORT_H

#include "kirc.h"

typedef struct {
    kirc_context_t *ctx;
    int fd;
} transport_t;

ssize_t transport_send(transport_t *transport,
        const char *buffer, size_t len);
ssize_t transport_receive(transport_t *transport,
        char *buffer, size_t len);

int transport_connect(transport_t *transport);
int transport_init(transport_t *transport,
        kirc_context_t *ctx);
int transport_free(transport_t *transport);

#endif  // __KIRC_TRANSPORT_H
