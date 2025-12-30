/*
 * output.c
 * Buffered output for improved terminal responsiveness
 * Author: Michael Czigler
 * License: MIT
 */

#include "output.h"

int output_init(struct output *output, struct kirc_context *ctx)
{
    if (output == NULL || ctx == NULL) {
        return -1;
    }

    memset(output, 0, sizeof(*output));

    output->ctx = ctx;
    output->len = 0;
    output->buffer[0] = '\0';

    return 0;
}

int output_append(struct output *output, const char *fmt, ...)
{
    if (output == NULL || fmt == NULL) {
        return -1;
    }
    
    /* Check if buffer has space for at least a reasonable message */
    if (output->len >= KIRC_OUTPUT_BUFFER_SIZE - 256) {
        /* Auto-flush if buffer is nearly full */
        output_flush(output);
    }
    
    va_list ap;
    va_start(ap, fmt);
    
    int remaining = KIRC_OUTPUT_BUFFER_SIZE - output->len;
    int written = vsnprintf(output->buffer + output->len,
                            remaining, fmt, ap);
    
    va_end(ap);
    
    if (written < 0) {
        return -1;
    }
    
    if (written >= remaining) {
        /* Message was truncated, flush and try again */
        output_flush(output);
        
        va_start(ap, fmt);
        written = vsnprintf(output->buffer, KIRC_OUTPUT_BUFFER_SIZE, fmt, ap);
        va_end(ap);
        
        if (written < 0 || written >= KIRC_OUTPUT_BUFFER_SIZE) {
            return -1;
        }
        
        output->len = written;
    } else {
        output->len += written;
    }
    
    return 0;
}

void output_flush(struct output *output)
{
    if (output == NULL || output->len == 0) {
        return;
    }
    
    /* Write entire buffer in one system call */
    ssize_t written = write(STDOUT_FILENO, output->buffer, output->len);
    
    (void)written;  /* Ignore errors - output is best-effort */
    
    output->len = 0;
    output->buffer[0] = '\0';
}

void output_clear(struct output *output)
{
    if (output == NULL) {
        return;
    }
    
    output->len = 0;
    output->buffer[0] = '\0';
}

int output_pending(struct output *output)
{
    if (output == NULL) {
        return 0;
    }
    
    return output->len;
}
