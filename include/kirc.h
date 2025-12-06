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

typedef struct {
    terminal_context_t terminal;
    utf8_context_t utf8;
    history_context_t history;
    editor_context_t editor;
    network_context_t network;
    parser_context_t parse;
    render_context_t render;
    logger_context_t logger;
} kirc_context_t;

#endif  // __KIRC_H
