/*
 * utf8.h
 * Header for the UTF-8 module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_UTF8_H
#define __KIRC_UTF8_H

#include "kirc.h"

int utf8_prev_char_len(const char *s, int pos);
int utf8_next_char_len(const char *s, int pos, int maxlen);
int utf8_validate(const char *s, int len);

#endif  // __KIRC_UTF8_H
