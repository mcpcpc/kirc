/*
 * kirc.h
 * Main header file for kirc IRC client
 * Author: Michael Czigler
 * License: MIT
 */

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

#ifndef NAME_MAX
#define NAME_MAX               255
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX          255
#endif

#define CHANNEL_MAX_LEN        200  /* per RFC1459 */
#define MESSAGE_MAX_LEN        512  /* per RFC1459 */
#define AUTH_CHUNK_SIZE        400  /* per IRCv3.1 */

#define KIRC_VERSION_MAJOR     "1"
#define KIRC_VERSION_MINOR     "0"
#define KIRC_VERSION_PATCH     "7"

#define KIRC_CHANNEL_LIMIT     256
#define KIRC_DCC_BUFFER_SIZE   8192
#define KIRC_DCC_TRANSFERS_MAX 16
#define KIRC_HISTORY_SIZE      64
#define KIRC_TAB_WIDTH         4
#define KIRC_TIMEOUT_MS        5000
#define KIRC_TIMESTAMP_SIZE    6
#define KIRC_TIMESTAMP_FORMAT  "%H:%M"

#define KIRC_DEFAULT_COLUMNS   80
#define KIRC_DEFAULT_PORT      "6667"
#define KIRC_DEFAULT_SERVER    "irc.libera.chat"

typedef enum {
    SASL_NONE = 0,
    SASL_PLAIN,
    SASL_EXTERNAL
} sasl_mechanism_t;

typedef struct {
    char server[HOST_NAME_MAX];
    char port[6];
    char nickname[MESSAGE_MAX_LEN];
    char realname[MESSAGE_MAX_LEN];
    char username[MESSAGE_MAX_LEN];
    char password[MESSAGE_MAX_LEN];
    char channels[KIRC_CHANNEL_LIMIT][CHANNEL_MAX_LEN];
    char target[KIRC_CHANNEL_LIMIT];
    char auth[MESSAGE_MAX_LEN];
    sasl_mechanism_t mechanism;
} kirc_t;

#endif  // __KIRC_H
