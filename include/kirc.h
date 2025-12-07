#ifndef __KIRC_H
#define __KIRC_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef RFC1459_CHANNEL_MAX_LEN
#define RFC1459_CHANNEL_MAX_LEN 200
#endif

#ifndef RFC1459_MESSAGE_MAX_LEN
#define RFC1459_MESSAGE_MAX_LEN 512
#endif

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

typedef struct {
    char hostname[HOST_NAME_MAX];
    char port[6];
    char nickname[128];
    char realname[128];
    char username[128];
    char password[128];
    char log[PATH_MAX];
    /* network */
    int socket_fd;
    /* terminal */
    int tty_fd;
    int raw_mode_enabled;
    struct termios original;
} kirc_t;

#endif  // __KIRC_H
