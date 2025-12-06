#include "history.h"

int history_init(history_context_t *hctx, int max_len)
{
    if (!hctx) {
        return -1;
    }

    hctx->items = NULL;
    hctx->length = 0;
    hctx->max_length = max_len;

    if (max_len <= 0) {
        return 0; /* history disabled */
    }

    hctx->items = calloc((size_t)max_len, sizeof(char *));
    if (!hctx->items) {
        hctx->max_length = 0;
        return -1;
    }

    return 0;
}

void history_free(history_context_t *hctx)
{
    if (!hctx) {
        return;
    }

    if (hctx->items) {
        for (int i = 0; i < hctx->length; ++i) {
            free(hctx->items[i]);
        }
        free(hctx->items);
    }

    hctx->items = NULL;
    hctx->length = 0;
    hctx->max_length = 0;
}

int history_add(history_context_t *hctx, const char *line)
{
    char *copy;

    if (!hctx || hctx->max_length == 0) {
        return 0;
    }

    if (!line) {
        line = "";
    }

    /* No array allocated (shouldn’t happen if history_init succeeded),
       but be robust. */
    if (!hctx->items) {
        hctx->items = calloc((size_t)hctx->max_length, sizeof(char *));
        if (!hctx->items) {
            hctx->max_length = 0;
            return 0;
        }
    }

    /* Avoid duplicating identical consecutive entries. */
    if (hctx->length > 0 &&
        hctx->items[hctx->length - 1] != NULL &&
        strcmp(hctx->items[hctx->length - 1], line) == 0) {
        return 0;
    }

    copy = strdup(line);
    if (!copy) {
        return 0;
    }

    if (hctx->length == hctx->max_length) {
        /* Drop oldest, shift everything left by one. */
        free(hctx->items[0]);
        memmove(hctx->items,
                hctx->items + 1,
                (size_t)(hctx->max_length - 1) * sizeof(char *));
        hctx->length--;
    }

    hctx->items[hctx->length] = copy;
    hctx->length++;

    return 1;
}

const char *history_get(history_context_t *hctx, int index)
{
    if (!hctx || !hctx->items) {
        return NULL;
    }
    if (index < 0 || index >= hctx->length) {
        return NULL;
    }
    return hctx->items[index];
}
