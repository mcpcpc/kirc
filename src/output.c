/*
 * output.c
 * Buffered output for improved terminal responsiveness
 * Author: Michael Czigler
 * License: MIT
 */

#include "output.h"

/**
 * output_init() - Initialize output buffer
 * @output: Output structure to initialize
 * @ctx: IRC context structure
 *
 * Initializes the output buffer system, preparing it for buffered writes
 * to stdout. Associates the output with an IRC context.
 *
 * Return: 0 on success, -1 if output or ctx is NULL
 */
int output_init(struct output *output,
        struct kirc_context *ctx)
{
    if ((output == NULL) || (ctx == NULL)) {
        return -1;
    }

    memset(output, 0, sizeof(*output));

    output->ctx = ctx;
    output->len = 0;
    output->buffer[0] = '\0';

    return 0;
}

/**
 * output_append() - Append formatted text to output buffer
 * @output: Output buffer structure
 * @fmt: printf-style format string
 * @...: Variable arguments for format string
 *
 * Appends formatted text to the output buffer. Automatically flushes if
 * buffer becomes nearly full (within 256 bytes of capacity). If text
 * doesn't fit, flushes buffer and retries. Handles truncation for
 * extremely long messages.
 *
 * Return: 0 on success, -1 on error
 */
int output_append(struct output *output,
        const char *fmt, ...)
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
        written = vsnprintf(output->buffer,
            KIRC_OUTPUT_BUFFER_SIZE, fmt, ap);
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

/**
 * output_flush() - Write buffered output to stdout
 * @output: Output buffer structure
 *
 * Writes the entire buffered content to stdout in a single system call
 * and resets the buffer. Improves performance by reducing write() calls.
 * Ignores write errors.
 */
void output_flush(struct output *output)
{
    if (output == NULL || output->len == 0) {
        return;
    }
    
    /* Write entire buffer in one system call */
    ssize_t written = write(STDOUT_FILENO,
        output->buffer, output->len);
    
    (void)written;  /* Ignore errors */
    
    output->len = 0;
    output->buffer[0] = '\0';
}

/**
 * output_clear() - Clear output buffer without writing
 * @output: Output buffer structure
 *
 * Discards all buffered content without writing it to stdout. Resets
 * the buffer to empty state.
 */
void output_clear(struct output *output)
{
    if (output == NULL) {
        return;
    }
    
    output->len = 0;
    output->buffer[0] = '\0';
}

/**
 * output_pending() - Check if output buffer has data
 * @output: Output buffer structure
 *
 * Returns the number of bytes currently buffered but not yet written
 * to stdout.
 *
 * Return: Number of buffered bytes, or 0 if output is NULL
 */
int output_pending(struct output *output)
{
    if (output == NULL) {
        return 0;
    }
    
    return output->len;
}
