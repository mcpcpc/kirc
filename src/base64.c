/*
 * base64.c
 * Base64 encoding handlers
 * Author: Michael Czigler
 * License: MIT
 */

#include "base64.h"

size_t base64_encoded_size(size_t len) {
    size_t ret = len;

    if (len % 3 != 0) {
        ret += 3 - (len % 3);
    }

    ret = (ret / 3) * 4;

    return ret;
}

int base64_encode(char *out, const char *in, size_t in_len)
{
    size_t i, j, v;

    if (in == NULL || in_len == 0 || out == NULL) {
        return -1;
    }

    for (i = 0, j = 0; i < in_len; i += 3, j += 4) {
        v = in[i];
        v = (i + 1) < in_len ? v << 8 | in[i + 1] : v << 8;
        v = (i + 2) < in_len ? v << 8 | in[i + 2] : v << 8;

        out[j] = base64_table[(v >> 18) & 0x3F];
        out[j + 1] = base64_table[(v >> 12) & 0x3F];

        if ((i + 1) < in_len) {
            out[j + 2] = base64_table[(v >> 6) & 0x3F];
        } else {
            out[j + 2] = '=';
        }

        if ((i + 2) < in_len) {
            out[j + 3] = base64_table[v & 0x3F];
        } else {
            out[j + 3] = '=';
        }
    }

    return 0;
}
