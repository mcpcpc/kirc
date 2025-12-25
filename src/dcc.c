/*
 * dcc.c
 * Direct client-to-client (DCC) handling
 * Author: Michael Czigler
 * License: MIT
 */

#include "dcc.h"

int dcc_init(dcc_t *dcc, kirc_t *ctx)
{
    if ((dcc == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(dcc, 0, sizeof(*dcc));
    dcc->ctx = ctx;
    dcc->transfer_count = 0;

    int limit = KIRC_DCC_TRANSFER_MAX;

    for (int i = 0; i < limit; ++i) {
        dcc->sock_fd[i].fd = -1;
        dcc->sock_fd[i].events = POLLIN;
        dcc->transfer[i].state = DCC_STATE_IDLE;
        dcc->transfer[i].file_fd = -1;
    }

    return 0;
}

int dcc_free(dcc_t *dcc)
{
    if (dcc == NULL) {
        return -1;
    }

    int limit = KIRC_DCC_TRANSFER_MAX;

    for (int i = 0; i < limit; ++i) {
        if (dcc->sock_fd[i].fd >= 0) {
            close(dcc->sock_fd[i].fd);
            dcc->sock_fd[i].fd = -1;
        }
        if (dcc->transfer[i].file_fd >= 0) {
            close(dcc->transfer[i].file_fd);
            dcc->transfer[i].file_fd = -1;
        }
    }

    return 0;
}
