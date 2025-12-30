/*
 * dcc.h
 * Header for the DCC module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_DCC_H
#define __KIRC_DCC_H

#include "kirc.h"
#include "ansi.h"
#include "helper.h"
#include "event.h"
#include "network.h"
#include "handler.h"

enum dcc_type {
    DCC_TYPE_SEND = 0,
    DCC_TYPE_RECEIVE
};

enum dcc_state {
    DCC_STATE_IDLE = 0,
    DCC_STATE_CONNECTING,
    DCC_STATE_TRANSFERRING,
    DCC_STATE_COMPLETE,
    DCC_STATE_ERROR
};

struct dcc_transfer {
    enum dcc_type type;
    enum dcc_state state;
    char filename[NAME_MAX];
    char sender[MESSAGE_MAX_LEN];
    unsigned long long filesize;
    unsigned long long sent;
    int file_fd;
};

struct dcc {
    struct kirc_context *ctx;
    struct pollfd sock_fd[KIRC_DCC_TRANSFERS_MAX];
    struct dcc_transfer transfer[KIRC_DCC_TRANSFERS_MAX];
    int transfer_count;
};

int dcc_init(struct dcc *dcc, struct kirc_context *ctx);
int dcc_free(struct dcc *dcc);
int dcc_request(struct dcc *dcc, const char *sender,
        const char *params);
int dcc_send(struct dcc *dcc, int transfer_id);
int dcc_process(struct dcc *dcc);
int dcc_cancel(struct dcc *dcc, int transfer_id);
void dcc_handle(struct dcc *dcc, struct network *network,
        struct event *event);

#endif  // __KIRC_DCC_H
