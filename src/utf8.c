/*
 * utf8.c
 * UTF-8 utilities
 * Author: Michael Czigler
 * License: MIT
 */
 
#include "utf8.h"

/**
 * utf8_prev_char_len() - Get byte length of previous UTF-8 character
 * @s: UTF-8 string
 * @pos: Current byte position in string
 *
 * Calculates the byte length of the UTF-8 character immediately before
 * the given position by walking backward to find the start byte. Handles
 * multi-byte sequences by identifying continuation bytes (0x80-0xBF).
 *
 * Return: Number of bytes in previous character, or 0 if at start
 */
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

/**
 * utf8_next_char_len() - Get byte length of next UTF-8 character
 * @s: UTF-8 string
 * @pos: Current byte position in string
 * @maxlen: Maximum valid position in string
 *
 * Determines the byte length of the UTF-8 character at the given position
 * using mbrtowc(). Handles invalid sequences by treating them as single
 * bytes. Safely handles null characters and position boundaries.
 *
 * Return: Number of bytes in next character (1-4), or 0 if at end
 */
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

/**
 * utf8_validate() - Validate UTF-8 encoding of string
 * @s: String to validate
 * @len: Number of bytes to validate
 *
 * Checks whether a byte sequence is valid UTF-8 by attempting to decode
 * each character using mbrtowc(). Rejects strings with invalid sequences,
 * incomplete multi-byte sequences, or other encoding errors.
 *
 * Return: 1 if valid UTF-8, 0 if invalid or incomplete
 */
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
