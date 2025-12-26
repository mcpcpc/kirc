/*
 * helper.c
 * Helper functions
 * Author: Michael Czigler
 * License: MIT
 */

#include "helper.h"

char * safecpy(char *s1, const char *s2, size_t n)
{
    char *out;
    
    out = strncpy(s1, s2, n);
    s1[n] = '\0';
 
    return out;
}
