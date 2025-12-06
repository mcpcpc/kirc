#include "log.h"

static const char *log_timestamp(char buf[26])
{
    time_t now;
    struct tm tm_;

    time(&now);

    if (!localtime_r(&now, &tm_)) {
        return NULL;
    }

    if (!asctime_r(&tm_, buf)) {
        return NULL;
    }

    /* Strip trailing newline */
    char *nl = strchr(buf, '\n');
    if (nl) {
        *nl = '\0';
    }

    return buf;
}

void log_append(log_t *log, const char *line)
{
    if (!log || !log->path || !line) {
        return;
    }

    FILE *out = fopen(log->path, "a");
    if (!out) {
        perror("log_append: fopen");
        return;
    }

    char ts[26];
    const char *stamp = log_timestamp(ts);

    if (stamp) {
        fprintf(out, "%s: ", stamp);
    }

    const unsigned char *p = (const unsigned char *)line;
    while (*p) {
        if ((*p >= 32 && *p < 127) || *p == '\n') {
            fputc(*p, out);
        }
        p++;
    }

    /* Ensure newline termination */
    if (line[strlen(line) - 1] != '\n') {
        fputc('\n', out);
    }

    fclose(out);
}
