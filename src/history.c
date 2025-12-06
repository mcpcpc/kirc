#include "history.h"

int history_init(history_t *h, int capacity)
{
    if (!h || capacity < 0) {
        return -1;
    }

    h->entries  = NULL;
    h->length   = 0;
    h->capacity = capacity;

    if (capacity == 0) {
        return 0;
    }

    h->entries = calloc((size_t)capacity, sizeof(char *));
    if (!h->entries) {
        h->capacity = 0;
        return -1;
    }

    return 0;
}

void history_free(history_t *h)
{
    if (!h) {
        return;
    }

    if (h->entries) {
        for (int i = 0; i < h->length; i++) {
            free(h->entries[i]);
        }
        free(h->entries);
    }

    h->entries  = NULL;
    h->length   = 0;
    h->capacity = 0;
}

int history_add(history_t *h, const char *line)
{
    char *copy;

    if (!h || !line || h->capacity == 0) {
        return 0;
    }

    /* Reject consecutive duplicates (like original code) */
    if (h->length > 0) {
        const char *last = h->entries[h->length - 1];
        if (last && strcmp(last, line) == 0) {
            return 0;
        }
    }

    copy = strdup(line);
    if (!copy) {
        return 0;
    }

    /* If full, evict oldest entry */
    if (h->length == h->capacity) {
        free(h->entries[0]);
        memmove(&h->entries[0],
                &h->entries[1],
                (size_t)(h->capacity - 1) * sizeof(char *));
        h->length--;
    }

    h->entries[h->length] = copy;
    h->length++;

    return 1;
}

const char *history_get(history_t *h, int index)
{
    int internal_index;

    if (!h || !h->entries || index < 0 || h->length == 0) {
        return NULL;
    }

    /* 0 = most recent => entries[length - 1] */
    internal_index = h->length - 1 - index;

    if (internal_index < 0 || internal_index >= h->length) {
        return NULL;
    }

    return h->entries[internal_index];
}
