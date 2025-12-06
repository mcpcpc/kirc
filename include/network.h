#ifndef __KIRC_NETWORK_H
#define __KIRC_NETWORK_H

#include "kirc.h"

typedef struct {
    int socket_fd;

    char *host;
    char *port;

    char *nick;
    char *user;
    char *real;
    char *pass;
    char *auth;

    int sasl_enabled;

    int has_initial_cmds;
    char init_cmd_buffer[1024];
    char *init_cmd_tail;
} network_t;

int network_connect(network_t *net);
void network_send(network_t *net, const char *fmt, ...);
int network_poll(network_t *net);

#endif  // __KIRC_NETWORK_H
