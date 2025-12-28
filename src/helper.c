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
