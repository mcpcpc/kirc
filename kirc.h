/*
 * SPDX-License-Identifier: MIT
 * Copyright (C) 2023 Michael Czigler
 */

#ifndef __KIRC_H
#define __KIRC_H

#define _POSIX_C_SOURCE 200809L
#define CTCP_CMDS "ACTION VERSION TIME CLIENTINFO PING DCC"
#define VERSION "0.3.2"
#define TESTCHARS "\xe1\xbb\xa4"
#define MSG_MAX 512 /* irc rfc says lines are 512 char's max, but servers can accept more */
#define CHA_MAX 200
#define WRAP_LEN 26
#define ABUF_LEN (sizeof("\r") - 1 + CHA_MAX + sizeof("> ") - 1 + MSG_MAX + sizeof("\x1b[0K") - 1 + 32 + 1)
                                                              /* this is as big as the ab buffer can get */
#define HIS_MAX 100
#define FNM_MAX 255
#define DIR_MAX 256
#define ERR_MAX 1 /* number of read/write errors before DCC slot is discarded */
#define POLL_TIMEOUT 180000 /* 3 minutes */ /* argument is in miliseconds */
#define CON_MAX 20
#define BACKLOG 100 /* DCC SEND listen() backlog */
#define CBUF_SIZ 1024
#define DCC_FLAGS (O_WRONLY | O_APPEND)
#define DCC_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
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
#include <signal.h>

static int conn;                /* connection socket */
static int hint_family = AF_UNSPEC; /* desired ip version */
static char verb = 0;           /* verbose output */
static char sasl = 0;           /* SASL method */
static char isu8 = 0;           /* UTF-8 flag */
static char dcc = 0;            /* DCC flag */
static char filter = 0;	        /* flag to filter ansi colors */
static char* dcc_dir = NULL;    /* DCC download directory */
static const char *host = "irc.libera.chat";  /* host address */
static const char *port = "6667";     /* port */
static char chan[MSG_MAX];      /* channel and prompt */
static char *nick = NULL;       /* nickname */
static char *pass = NULL;       /* password */
static char *user = NULL;       /* user name */
static char *auth = NULL;       /* PLAIN SASL token */
static char *real = NULL;       /* real name */
static char *olog = NULL;       /* chat log path */
static char *inic = NULL;       /* additional server command */
static char cmds = 0;           /* indicates additional server commands */
static char cbuf[CBUF_SIZ];     /* additional stdin server commands */

/* define function macros */
#define htonll(x) ((1==htonl(1)) ? (x) : (((uint64_t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32_t)((x) >> 32)))
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#undef htonll
#define htonll(x) ((x))
#endif
#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#undef htonll
#define htonll(x) ((((uint64_t)htonl((x) & 0xFFFFFFFFUL)) << 32) | htonl((uint32_t)((x) >> 32)))
#endif

#define buffer_position_move(l, dest, src, size) memmove(l->buf + l->posb + dest, l->buf + l->posb + src, size)
#define buffer_position_move_end(l, dest, src) buffer_position_move(l, dest, src, l->lenb - (l->posb + src) + 1)
#define u8_character_start(c) (isu8 ? (c & 0x80) == 0x00 || (c & 0xC0) == 0xC0 : 1)

static int ttyinfd = STDIN_FILENO;
static struct termios orig;
static int history_len = 0;
static char history_wrap = 0;
static char history[HIS_MAX][MSG_MAX];
static char small_screen;

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
    char buf[MSG_MAX];          /* Edited line buffer. */
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
    char nick_privmsg;          /* whether or not we are sending messages to a chan or nick */
    char chan_privmsg;          /* flag to toggle raw messages */
} state_t, *state;

struct abuf {
    char b[ABUF_LEN];
    int len;
};

union sockaddr_in46 {
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
    sa_family_t sin_family;
};

struct dcc_connection {
    unsigned long long bytes_read;
    unsigned long long file_size;
    union sockaddr_in46 sin46;
    int file_fd;
    int err_cnt;
    int write;
    char filename[FNM_MAX + 1];
};

static struct {
    struct pollfd sock_fds[CON_MAX + 2];
    struct dcc_connection slots[CON_MAX];
} dcc_sessions = {0};

#endif
