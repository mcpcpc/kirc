/*
 * helper.c
 * Helper functions
 * Author: Michael Czigler
 * License: MIT
 */

#include "helper.h"

int safecpy(char *s1, const char *s2, size_t n)
{
    if (n == 0) {
        return -1;
    }
    
    strncpy(s1, s2, n - 1);
    s1[n - 1] = '\0';
    
    return 0;
}

/*
int secure_zero(void *ptr, size_t n)
{
    if ((ptr == NULL) || (n == 0)) {
        return -1;
    }

    // use volatile to prevent compiler optimization
    volatile unsigned char *p = ptr;

    while (n--) {
        *p++ = 0;
    }
    
    return 0;
}
*/
int memzero(void *s, size_t n)
{
    if ((s == NULL) || (n == 0)) {
        return -1;
    }

    if (memset(s, 0, n) == NULL) {
        return -1;
    }
    
    return 0;
}
