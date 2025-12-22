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
#define KIRC_HISTORY_SIZE 8
#endif

#ifndef KIRC_TIMEOUT_MS
#define KIRC_TIMEOUT_MS 5000
#endif

#ifndef KIRC_TIMESTAMP_FORMAT
#define KIRC_TIMESTAMP_FORMAT "%H:%M"
#endif

#ifndef KIRC_DEFAULT_PORT
#define KIRC_DEFAULT_PORT "6667"
#endif

#ifndef KIRC_DEFAULT_SERVER
#define KIRC_DEFAULT_SERVER "irc.libera.chat"
#endif

#ifndef KIRC_DEFAULT_COLUMNS
#define KIRC_DEFAULT_COLUMNS 80
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
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

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

typedef enum {
    SASL_NONE = 0,
    SASL_PLAIN,
    SASL_EXTERNAL
} sasl_mechanism_t;

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
    sasl_mechanism_t mechanism;
} kirc_t;

#endif  // __KIRC_H
