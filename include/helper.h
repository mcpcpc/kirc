/*
 * helper.h
 * Header for the helper module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_HELPER_H
#define __KIRC_HELPER_H

#include "kirc.h"

/* Safe string copy - always null-terminates */
int safecpy(char *s1, const char *s2, size_t n);

/* Securely zero memory to prevent sensitive data leakage */
int memzero(void *s, size_t n);

#endif  // __KIRC_HELPER_H
