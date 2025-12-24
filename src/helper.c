/*
 * helper.c
 * Helper functions for the IRC client
 * Author: Michael Czigler
 * License: MIT
 */

#include "helper.h"

int safecpy(const char *s1, char *s2)
{
    size_t n = sizeof(s1) - 1;
    char *r = strncpy(s1, s2, siz);

    if (r == NULL) {
        return 1;
    }

    s1[n] = '\0';

    return 0;
}
