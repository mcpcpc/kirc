/*
 * output.h
 * Header for the buffered output module
 * Author: Michael Czigler
 * License: MIT
 */

#ifndef __KIRC_OUTPUT_H
#define __KIRC_OUTPUT_H

#include "kirc.h"
#include "ansi.h"

struct output {
    struct kirc_context *ctx;
    char buffer[KIRC_OUTPUT_BUFFER_SIZE];
    int len;
};

int output_init(struct output *output,
        struct kirc_context *ctx);

int output_append(struct output *output,
        const char *fmt, ...);

void output_flush(struct output *output);
void output_clear(struct output *output);

int output_pending(struct output *output);

#endif  // __KIRC_OUTPUT_H
