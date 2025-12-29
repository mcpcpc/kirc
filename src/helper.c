/*
 * helper.c
 * Helper functions
 * Author: Michael Czigler
 * License: MIT
 */

#include "helper.h"

int safecpy(char *s1, const char *s2, size_t n)
{
    if ((s1 == NULL) || (s2 == NULL) || (n == 0)) {
        return -1;
    }
    
    strncpy(s1, s2, n - 1);
    s1[n - 1] = '\0';
    
    return 0;
}

int memzero(void *s, size_t n)
{
    if ((s == NULL) || (n == 0)) {
        return -1;
    }

    /* use volatile to prevent compiler optimization */
    volatile unsigned char *p = s;

    while (n--) {
        *p++ = 0;
    }
    
    return 0;
}

char *find_message_end(const char *buffer, size_t len)
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

