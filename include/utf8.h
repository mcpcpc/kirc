#ifndef __KIRC_UTF8_H
#define __KIRC_UTF8_H

#include "kirc.h"

typedef struct {
    int enabled;
} utf8_context_t;

int utf8_detect(utf8_context_t *utf8, int input_fd,
        int output_fd);

int utf8_is_char_start(utf8_context_t *utf8, char c);
int utf8_char_size(utf8_context_t *utf8, char c);

size_t utf8_strlen(utf8_context_t *utf8, const char *s);
size_t utf8_prev(utf8_context_t *utf8, const char *s,
        size_t byte_pos);
size_t utf8_next(utf8_context_t *utf8, const char *s,
        size_t byte_pos);

#endif  // __KIRC_UTF8_H
