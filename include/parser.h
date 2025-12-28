/*
 * parser.h
 * IRC message parsing utilities
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_PARSER_H
#define __KIRC_PARSER_H

#include "kirc.h"

/* Find the end of an IRC message in a buffer, respecting CTCP markers 
 * Returns pointer to '\r' or NULL if no complete message found */
char *parser_find_message_end(const char *buffer, size_t len);

#endif  // __KIRC_PARSER_H
