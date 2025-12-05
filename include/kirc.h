#ifndef __KIRC_H
#define __KIRC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#ifndef RFC1459_MESSAGE_MAX_LEN
#define RFC1459_MESSAGE_MAX_LEN 512
#endif

#ifndef RFC1459_CHANNEL_MAX_LEN
#define RFC1459_CHANNEL_MAX_LEN 200
#endif

typedef enum {
    KIRC_OK = 0,
    KIRC_ERROR_MEMORY,
    KIRC_ERROR_NETWORK,
    KIRC_ERROR_PROTOCOL,
    KIRC_ERROR_AUTHENTICATION,
} kirc_error_t;

typedef struct {
    int utf8_enabled;
} kirc_context_t

#endif  // __KIRC_H
