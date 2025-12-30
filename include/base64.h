/*
 * base64.h
 * Header for base64 encoding module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_BASE64_H
#define __KIRC_BASE64_H

#include "kirc.h"

const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Calculate the size of the encoded data based on input length */
size_t base64_encoded_size(size_t len);

/* Encode input data to base64 format */
int base64_encode(const uint8_t *in, size_t in_len, char *out, size_t out_size);

#endif // __KIRC_BASE64_H
