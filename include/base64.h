/*
 * base64.h
 * Header for base64 encoding module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_BASE64_H
#define __KIRC_BASE64_H

#include "kirc.h"

/* Calculate the size of the encoded data based on input length */
size_t base64_encoded_size(size_t len);

/* Encode input data to base64 format */
int base64_encode(char *out, const char *in, size_t in_len);

#endif // __KIRC_BASE64_H
