#ifndef __KIRC_PARSE_H
#define __KIRC_PARSE_H

#include "kirc.h"
#include "event.h"

typedef struct {
    int dummy; /* future parser state, if needed */
} parser_t;

int parser_feed(parser_t *p, char *line, event_t *out_event);

#endif  // __KIRC_PARSE_H
