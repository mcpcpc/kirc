#ifndef __KIRC_H
#define __KIRC_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>

typedef struct {
    char hostname[HOST_NAME_MAX];
    char port[6];
    char nickname[128];
    char realname[128];
    char username[128];
    char password[128];
    char log[PATH_MAX];

    int conn;
} kirc_t;

#endif  // __KIRC_H
