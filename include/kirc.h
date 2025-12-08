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

#ifndef KIRC_CHANLIMIT
#define KIRC_CHANLIMIT 100
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
#include <unistd.h>
#include <wchar.h>
#include <wctype.h>

typedef struct {
    char hostname[HOST_NAME_MAX];
    char port[6];
    char nickname[RFC1459_MESSAGE_MAX_LEN];
    char realname[RFC1459_MESSAGE_MAX_LEN];
    char username[RFC1459_MESSAGE_MAX_LEN];
    char password[RFC1459_MESSAGE_MAX_LEN];
    char channel[KIRC_CHANLIMIT][RFC1459_CHANNEL_MAX_LEN];
    /* network */
    int socket_fd;
    int socket_len;
    char socket_buffer[RFC1459_MESSAGE_MAX_LEN];
    /* terminal */
    int tty_fd;
    int lwidth;
    int raw_mode_enabled;
    struct termios original;
} kirc_t;

#endif  // __KIRC_H
