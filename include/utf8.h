#ifndef __KIRC_UTF8_H
#define __KIRC_UTF8_H

#include "kirc.h"

typedef struct {
    int enabled;
} utf8_t;

int utf8_detect(utf8_t *u, int in_fd, int out_fd);

int utf8_is_start(utf8_t *u, char c);
int utf8_char_size(utf8_t *u, char c);

size_t utf8_length(utf8_t *u, const char *s);
size_t utf8_prev(utf8_t *u, const char *s, size_t pos);
size_t utf8_next(utf8_t *u, const char *s, size_t pos);

#endif  // __KIRC_UTF8_H
