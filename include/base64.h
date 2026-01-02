/*
 * base64.h
 * Header for base64 encoding module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_BASE64_H
#define __KIRC_BASE64_H

#include "kirc.h"

int base64_encode(char *out, const char *in, size_t in_len);

#endif // __KIRC_BASE64_H
