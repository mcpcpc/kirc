#ifndef __KIRC_H
#define __KIRC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    char *hostname;
    char *port;
    char *nickname;
    char *realname;
    char *username;
    char *password;
    char *token;
    char *log;
} kirc_t;

#endif  // __KIRC_H
