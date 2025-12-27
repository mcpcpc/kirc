/*
 * helper.h
 * Header for the helper module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_HELPER_H
#define __KIRC_HELPER_H

#include "kirc.h"

int safecpy(char *s1, const char *s2, size_t n);
int memzero(void *s, size_t n);

#endif  // __KIRC_HELPER_H
