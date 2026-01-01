/*
 * helper.c
 * Helper functions
 * Author: Michael Czigler
 * License: MIT
 */

#include "helper.h"

/**
 * safecpy() - Safe string copy with guaranteed null termination
 * @s1: Destination buffer
 * @s2: Source string
 * @n: Size of destination buffer
 *
 * Copies a string ensuring the destination is always null-terminated.
 * Safer alternative to strncpy. Copies at most n-1 characters and
 * always adds a null terminator.
 *
 * Return: 0 on success, -1 if parameters are invalid
 */
int safecpy(char *s1, const char *s2, size_t n)
{
    if ((s1 == NULL) || (s2 == NULL) || (n == 0)) {
        return -1;
    }
    
    strncpy(s1, s2, n - 1);
    s1[n - 1] = '\0';
    
    return 0;
}

/**
 * memzero() - Securely zero memory
 * @s: Memory buffer to zero
 * @n: Number of bytes to zero
 *
 * Securely overwrites memory with zeros, preventing compiler optimization
 * from removing the operation. Used for clearing sensitive data like
 * passwords. Uses volatile pointer to ensure the write is not optimized away.
 *
 * Return: 0 on success, -1 if parameters are invalid
 */
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

/**
 * find_message_end() - Find IRC message terminator in buffer
 * @buffer: Buffer containing IRC protocol data
 * @len: Length of the buffer
 *
 * Searches for the IRC message delimiter (\r\n) while properly handling
 * CTCP sequences. CTCP commands are delimited by \001 characters, and
 * message terminators within CTCP sequences are ignored. This prevents
 * premature message splitting when CTCP data contains \r\n.
 *
 * Return: Pointer to \r character of the terminator, or NULL if not found
 */
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
}
