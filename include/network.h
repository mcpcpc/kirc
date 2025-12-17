#ifndef __KIRC_NETWORK_H
#define __KIRC_NETWORK_H

#include "kirc.h"

typedef struct {
    kirc_t *ctx;
    char buffer[RFC1459_MESSAGE_MAX_LEN];
    int len;
    int fd;
} network_t;

void network_send(network_t *network, const char *fmt, ...);

int network_receive(network_t *network);
int network_connect(network_t *network);
int network_command_handler(network_t *network, char *msg);
int network_authenticate_plain(network_t *network, char *data);
int network_authenticate_external(network_t *network);
int network_init(network_t *network, kirc_t *ctx);
int network_free(network_t *network);

#endif  // __KIRC_NETWORK_H
