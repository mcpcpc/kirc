#ifndef __KIRC_H
#define __KIRC_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#ifndef RFC1459_CHANNEL_MAX_LEN
#define RFC1459_CHANNEL_MAX_LEN 200
#endif

#ifndef RFC1459_MESSAGE_MAX_LEN
#define RFC1459_MESSAGE_MAX_LEN 512
#endif

#ifndef IRCV3_AUTHENTICATE_CHUNK_SIZE
#define IRCV3_AUTHENTICATE_CHUNK_SIZE 400
#endif

#ifndef KIRC_CHAN_LIMIT
#define KIRC_CHAN_LIMIT 256
#endif

#ifndef KIRC_HISTORY_SIZE
#define KIRC_HISTORY_SIZE 32
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

typedef enum {
    SASL_NONE = 0,
    SASL_PLAIN,
    SASL_EXTERNAL
} mechanism_t;

typedef enum {
    KIRC_OK = 0,
    KIRC_ERR,
    KIRC_ERR_NOTFOUND,
    KIRC_ERR_IO,
    KIRC_ERR_PARSE,
    KIRC_ERR_INTERNAL
} kirc_error_t;

typedef struct {
    char server[HOST_NAME_MAX];
    char port[6];
    char nickname[RFC1459_MESSAGE_MAX_LEN];
    char realname[RFC1459_MESSAGE_MAX_LEN];
    char username[RFC1459_MESSAGE_MAX_LEN];
    char password[RFC1459_MESSAGE_MAX_LEN];
    char channels[KIRC_CHAN_LIMIT][RFC1459_CHANNEL_MAX_LEN];
    char selected[KIRC_CHAN_LIMIT];
    char auth[RFC1459_MESSAGE_MAX_LEN];
    mechanism_t mechanism;
    int filtered;
} kirc_t;

#endif  // __KIRC_H
