#ifndef __KIRC_NETWORK_H
#define __KIRC_NETWORK_H

#include "kirc.h"

void network_send(kirc_t *ctx, const char *fmt, ...);
void network_poll(kirc_t *ctx);

int network_connect(kirc_t *ctx);


#endif  // __KIRC_NETWORK_H
