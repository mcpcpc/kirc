#ifndef __KIRC_H
#define __KIRC_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#ifndef RFC1459_MESSAGE_MAX_LEN
#define RFC1459_MESSAGE_MAX_LEN 512
#endif

#ifndef RFC1459_CHANNEL_MAX_LEN
#define RFC1459_CHANNEL_MAX_LEN 200
#endif

#endif  // __KIRC_H
