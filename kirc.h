/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2023 Michael Czigler
 */

#ifndef __KIRC_H
#define __KIRC_H

#define _POSIX_C_SOURCE 200809L
#define CTCP_CMDS "ACTION VERSION TIME CLIENTINFO PING DCC"
#define VERSION "0.3.2"
#define MSG_MAX 512
#define CHA_MAX 200
#define NIC_MAX 26
#define HIS_MAX 100
#define FNM_MAX 255
#define CON_MAX 10
#define CBUF_SIZ 1024
#define STR_(a) #a
#define STR(a) STR_(a)

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include <stdarg.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <termios.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static char cdef[MSG_MAX] = ""; /* default PRIVMSG channel */
static int conn;                /* connection socket */
static int verb = 0;            /* verbose output */
static int sasl = 0;            /* SASL method */
static int isu8 = 0;            /* UTF-8 flag */
static const char *host = "irc.libera.chat";  /* host address */
static const char *port = "6667";     /* port */
static char *chan = NULL;       /* channel(s) */
static char *nick = NULL;       /* nickname */
static char *pass = NULL;       /* password */
static char *user = NULL;       /* user name */
static char *auth = NULL;       /* PLAIN SASL token */
static char *real = NULL;       /* real name */
static char *olog = NULL;       /* chat log path */
static char *inic = NULL;       /* additional server command */
static int cmds = 0;            /* indicates additional server commands */
static char cbuf[CBUF_SIZ];     /* additional stdin server commands */

static int ttyinfd = STDIN_FILENO;
static struct termios orig;
static int rawmode = 0;
static int atexit_registered = 0;
static int history_max_len = HIS_MAX;
static int history_len = 0;
static char **history = NULL;

typedef struct PARAMETERS {
    char *prefix;
    char *suffix;
    char *message;
    char *nickname;
    char *command;
    char *channel;
    char *params;
    size_t offset;
    size_t maxcols;
    int nicklen;
} param_t, *param;

typedef struct STATE {
    char *prompt;               /* Prompt to display. */
    char *buf;                  /* Edited line buffer. */
    size_t buflen;              /* Edited line buffer size. */
    size_t plenb;               /* Prompt length. */
    size_t plenu8;              /* Prompt length. */
    size_t posb;                /* Current cursor position. */
    size_t posu8;               /* Current cursor position. */
    size_t oldposb;             /* Previous refresh cursor position. */
    size_t oldposu8;            /* Previous refresh cursor position. */
    size_t lenb;                /* Current edited line length. */
    size_t lenu8;               /* Current edited line length. */
    size_t cols;                /* Number of columns in terminal. */
    int history_index;          /* Current line in the edit history */
} state_t, *state;

struct abuf {
    char *b;
    int len;
};

struct dcc_connection {
    char filename[FNM_MAX + 1];
    struct sockaddr_in sin;
    size_t bytes_read;
    size_t file_size;
    int file_fd;
};

static struct {
    struct pollfd sock_fds[CON_MAX + 2];
    struct dcc_connection slots[CON_MAX];
} dcc_sessions = {0};

#endif
