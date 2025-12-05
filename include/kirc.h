#ifndef __KIRC_H
#define __KIRC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#ifndef RFC1459_LINE_MAX_LEN
#define RFC1459_LINE_MAX_LEN 512
#endif

#ifndef KIRC_SASL_TOKEN_MAX_LEN
#define KIRC_SASL_TOKEN_MAX_LEN 20
#endif

#ifndef KIRC_HISTORY_MAX_N
#define KIRC_HISTORY_MAX_N 100
#endif

#ifndef KIRC_POLL_TIMEOUT_MS
#define KIRC_POLL_TIMEOUT_MS 180000
#endif

typedef enum {
    KIRC_OK = 0,
    KIRC_ERROR_MEMORY,
    KIRC_ERROR_NETWORK,
    KIRC_ERROR_PROTOCOL,
    KIRC_ERROR_AUTHENTICATION,
} kirc_error_t;

typedef struct {
    char history[KIRC_HISTORY_MAX_N][RFC1459_LINE_MAX_LEN];
    char sasl_token[KIRC_SASL_TOKEN_MAX_LEN];
    int family_hint;
} kirc_context_t

#endif // KIRC_H
