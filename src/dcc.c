/*
 * dcc.c
 * Direct client-to-client (DCC) handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "dcc.h"

int dcc_init(dcc_t *dcc, kirc_t *ctx) {
    if ((dcc == NULL) || (ctx == NULL)) {
        return -1;
    }
    
    memset(dcc, 0, sizeof(*dcc));

    dcc->ctx = ctx;
    dcc->count = 0;

    int lim = KIRC_DCC_TRANSFERS_MAX;
    for (int i = 0; i < lim; ++i) {
        dcc->transfer[i].sockfd = -1;
        dcc->transfer[i].filefd = -1;
        dcc->state = DCC_STATE_IDLE;
    }
    
    return 0;
}

int dcc_free(dcc_t *dcc) {
    if (dcc == NULL) {
        return -1;
    }

    int lim = KIRC_DCC_TRANSFERS_MAX;
    for (int i = 0; i < lim; ++i) {
        if (dcc->transfer[i].sockfd > 0) {
            close(dcc->transfer[i].sockfd);
        }
        if (dcc->transfer[i].filefd > 0) {
            close(dcc->transfer[i].filefd);
        }
    }

    return 0;
}
