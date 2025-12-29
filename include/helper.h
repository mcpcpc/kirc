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

/* Find the end of an IRC message in a buffer, respecting CTCP markers 
 * Returns pointer to '\r' or NULL if no complete message found */
char *find_message_end(const char *buffer, size_t len);

#endif  // __KIRC_HELPER_H
