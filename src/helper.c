/*
 * helper.c
 * Helper functions
 * Author: Michael Czigler
 * License: MIT
 */

#include "helper.h"

char * safecpy(char *s1, const char *s2, size_t n)
{
    if (n == 0) {
        return s1;
    }
    
    char *out = strncpy(s1, s2, n);
    s1[n] = '\0';
 
    return out;
}

int secure_zero(void *ptr, size_t n)
{
    if ((ptr == NULL) || (n == 0)) {
        return -1;
    }

    /* use volatile to prevent compiler optimization */
    volatile unsigned char *p = ptr;

    while (n--) {
        *p++ = 0;
    }
    
    return 0;
}