/*
 * utf8.c
 * UTF-8 utilities
 * Author: Michael Czigler
 * License: MIT
 */
 
#include "utf8.h"

int utf8_prev_char_len(const char *s, int pos)
{
    if (pos <= 0) {
        return 0;
    }

    /* Walk back to find start of UTF-8 character */
    int prev = pos - 1;
    while (prev > 0 && ((unsigned char)s[prev] & 0xC0) == 0x80) {
        prev--;
    }

    return pos - prev;
}

int utf8_next_char_len(const char *s, int pos, int maxlen)
{
    if (pos >= maxlen) {
        return 0;
    }

    mbstate_t st;
    memset(&st, 0, sizeof(st));
    
    wchar_t wc;
    size_t ret = mbrtowc(&wc, s + pos, maxlen - pos, &st);
    
    if (ret == (size_t)-1 || ret == (size_t)-2) {
        /* Invalid UTF-8 sequence, treat as single byte */
        return 1;
    }
    
    if (ret == 0) {
        /* Null character */
        return 1;
    }
    
    return (int)ret;
}

int utf8_validate(const char *s, int len)
{
    mbstate_t st;
    memset(&st, 0, sizeof(st));
    int pos = 0;
    
    while (pos < len) {
        wchar_t wc;
        size_t ret = mbrtowc(&wc, s + pos, len - pos, &st);
        
        if (ret == (size_t)-1) {
            /* Invalid sequence */
            return 0;
        }
        
        if (ret == (size_t)-2) {
            /* Incomplete sequence at end */
            return 0;
        }
        
        if (ret == 0) {
            /* Null encountered */
            break;
        }
        
        pos += ret;
    }
    
    return 1;
}
