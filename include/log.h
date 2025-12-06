#ifndef __KIRC_LOG_H
#define __KIRC_LOG_H

#include "kirc.h"

typedef struct {
    char *path;
} log_t;

void log_append(log_t *log, const char *line);

#endif  // __KIRC_LOG_H