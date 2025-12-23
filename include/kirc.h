#ifndef __KIRC_H
#define __KIRC_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
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
#define HOST_NAME_MAX           255
#endif

#define RFC1459_CHANNEL_MAX_LEN 200
#define RFC1459_MESSAGE_MAX_LEN 512

#define AUTHENTICATE_CHUNK_SIZE 400

#define KIRC_CHANNEL_LIMIT      256
#define KIRC_HISTORY_SIZE       8
#define KIRC_TIMEOUT_MS         5000
#define KIRC_DEFAULT_COLUMNS    80
#define KIRC_DEFAULT_PORT       "6667"
#define KIRC_DEFAULT_SERVER     "irc.libera.chat"
#define KIRC_TIMESTAMP_FORMAT   "%H:%M"

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
    char channels[KIRC_CHANNEL_LIMIT][RFC1459_CHANNEL_MAX_LEN];
    char selected[KIRC_CHANNEL_LIMIT];
    char auth[RFC1459_MESSAGE_MAX_LEN];
    sasl_mechanism_t mechanism;
} kirc_t;

#endif  // __KIRC_H
