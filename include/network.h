#ifndef __KIRC_NETWORK_H
#define __KIRC_NETWORK_H

#include "kirc.h"

void network_send(kirc_t *ctx, const char *fmt, ...);
int network_connect(kirc_t *ctx);

#endif  // __KIRC_NETWORK_H
