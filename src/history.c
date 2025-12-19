#include "history.h"

int history_init(history_t *history)
{
    memset(history, 0, sizeof(*history));

    history->count = 0;
    history->head = 0;
    history->position = -1;

    return 0;
}

int history_add(history_t *history, const char *entry)
{
    if (!entry || entry[0] == '\0') {
        return 0;  /* nothing to add */
    }

    int siz = sizeof(history->buffer[0]) - 1;
    strncpy(history->buffer[history->head], entry, siz);
    history->buffer[history->head][siz] = '\0';

    history->head = (history->head + 1) % KIRC_HISTORY_SIZE;
    if (history->count < KIRC_HISTORY_SIZE) {
        history->count++;
    }
    history->position = -1;

    return 1;
}

int history_previous(history_t *history, char *scratch)
{
    int siz = KIRC_HISTORY_SIZE;

    if (history->count == 0) {
        return -1;
    }

    int head = history->head;
    int oldest = (head - history->count + siz) % siz;
    int newest = (head - 1 + siz) % siz;

    int pos;
    if (history->position == -1) {
        pos = newest;
    } else if (history->position == oldest) {
        return -1; /* already at oldest */
    } else {
        pos = (history->position - 1 + siz) % siz;
    }

    strncpy(scratch, history->buffer[pos],
        RFC1459_MESSAGE_MAX_LEN - 1);
    scratch[RFC1459_MESSAGE_MAX_LEN - 1] = '\0';
    history->position = pos;

    return pos;
}

int history_next(history_t *history, char *scratch)
{
    int siz = KIRC_HISTORY_SIZE;

    if (history->position == -1) {
        return -1; /* not browsing */
    }

    int head = history->head;
    int newest = (head - 1 + siz) % siz;

    if (history->position == newest) {
        scratch[0] = '\0';
        history->position = -1;
        return -1;
    }

    int pos = (history->position + 1) % siz;

    strncpy(scratch, history->buffer[pos],
        RFC1459_MESSAGE_MAX_LEN - 1);
    scratch[RFC1459_MESSAGE_MAX_LEN - 1] = '\0';
    history->position = pos;

    return pos;
}

char *history_get_last(history_t *history)
{
    if (history->count == 0) {
        return NULL;
    }

    int siz = KIRC_HISTORY_SIZE;
    int last = (history->head - 1 + siz) % siz;
    return history->buffer[last];
}
