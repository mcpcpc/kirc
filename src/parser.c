/*
 * parser.c
 * IRC message parsing utilities
 * Author: Michael Czigler
 * License: MIT
 */

#include "parser.h"

char *parser_find_message_end(const char *buffer, size_t len)
{
    int ctcp_active = 0;

    for (size_t i = 0; i + 1 < len; ++i) {
        if (buffer[i] == '\001') {
            /* Toggle CTCP state when marker encountered */
            ctcp_active = !ctcp_active;
        } else if (buffer[i] == '\r' && buffer[i + 1] == '\n') {
            /* Message end found only if not inside CTCP sequence */
            if (!ctcp_active) {
                return (char *)(buffer + i);
            }
        }
    }

    return NULL;
}
