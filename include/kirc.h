#ifndef __KIRC_H
#define __KIRC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <fcntl.h>

#ifndef RFC1459_MESSAGE_MAX_LEN
#define RFC1459_MESSAGE_MAX_LEN 512
#endif

#ifndef RFC1459_CHANNEL_MAX_LEN
#define RFC1459_CHANNEL_MAX_LEN 200
#endif

#endif  // __KIRC_H
