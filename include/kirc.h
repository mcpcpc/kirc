#ifndef __KIRC_H
#define __KIRC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char hostname[HOST_NAME_MAX];
    char port[6];
    char *nickname;
    char *realname;
    char *username;
    char *password;
    char *token;
    char log[PATH_MAX];
} kirc_t;

#endif  // __KIRC_H
