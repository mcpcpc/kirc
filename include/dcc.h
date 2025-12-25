/*
 * dcc.h
 * Header for the DCC module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_DCC_H
#define __KIRC_DCC_H

#include "kirc.h"

typedef enum {
    DCC_TYPE_SEND = 0,
    DCC_TYPE_RECEIVE
} dcc_type_t;

typedef enum {
    DCC_STATE_IDLE = 0,
    DCC_STATE_CONNECTING,
    DCC_STATE_TRANSFERRING,
    DCC_STATE_COMPLETE,
    DCC_STATE_ERROR
} dcc_state_t;

typedef struct {
    dcc_type_t type;
    dcc_state_t state;
    char filename[NAME_MAX];
    char sender[MESSAGE_MAX_LEN];
    uint32_t size;
    uint32_t sent;
    struct pollfd sockfd;
    int filefd;
} dcc_transfer_t;

typedef struct {
    kirc_t *ctx;
    dcc_transfer_t transfer[KIRC_DCC_TRANSFERS_MAX];
    int count;
} dcc_t;

int dcc_init(dcc_t *dcc, kirc_t *ctx);
int dcc_free(dcc_t *dcc);
int dcc_request(dcc_t *dcc, const char *sender,
        const char *params);
int dcc_send(dcc_t *dcc, int transfer_id);
int dcc_process(dcc_t *dcc);
int dcc_cancel(dcc_t *dcc, int transfer_id);

#endif  // __KIRC_DCC_H
