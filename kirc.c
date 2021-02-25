#include <unistd.h>
#include "kirc.h"

static size_t strlen_c(char * str) {
    size_t n = 0;
    char * p = str;
    while ((* (p++)) != 0) {
        ++n;
    }
    return n;
}

static int strcmp_c(char * str1, char * str2) {
    char * c1 = str1;
    char * c2 = str2;
    while ((* c1) && ((* c1) == (* c2))) {
        ++c1;
        ++c2;
    }
    int n = (* c1) - (* c2);
    return n;
}

static void abInit(struct abuf * ab) {
    ab->b = NULL;
    ab->len = 0;
}

static void abAppend(struct abuf * ab, const char * s, int len) {
    char * new = realloc(ab->b, ab->len + len);
    if (new == NULL) {
        return;
    }
    memcpy(new + ab->len, s, len);
    ab->b = new;
    ab->len += len;
}

static void abFree(struct abuf * ab) {
    free(ab->b);
}

static void editMoveLeft(struct State * l) {
    if (l->pos > 0) {
        l->pos--;
        refreshLine(l);
    }
}

static void editMoveRight(struct State * l) {
    if (l->pos != l->len) {
        l->pos++;
        refreshLine(l);
    }
}

static void editMoveHome(struct State * l) {
    if (l->pos != 0) {
        l->pos = 0;
        refreshLine(l);
    }
}

static void editMoveEnd(struct State * l) {
    if (l->pos != l->len) {
        l->pos = l->len;
        refreshLine(l);
    }
}

static void editDelete(struct State * l) {
    if (l->len > 0 && l->pos < l->len) {
        memmove(l->buf + l->pos, l->buf + l->pos + 1, l->len - l->pos - 1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void editBackspace(struct State * l) {
    if (l->pos > 0 && l->len > 0) {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void editDeletePrevWord(struct State * l) {
    size_t old_pos = l->pos;
    while (l->pos > 0 && l->buf[l->pos - 1] == ' ') {
        l->pos--;
    }
    while (l->pos > 0 && l->buf[l->pos - 1] != ' ') {
        l->pos--;
    }
    size_t diff = old_pos - l->pos;
    memmove(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
    l->len -= diff;
    refreshLine(l);
}

static void editDeleteWholeLine(struct State * l) {
    l->buf[0] = '\0';
    l->pos = l->len = 0;
    refreshLine(l);
}

static void editDeleteLineToEnd(struct State * l) {
    l->buf[l->pos] = '\0';
    l->len = l->pos;
    refreshLine(l);
}

static void editSwapCharWithPrev(struct State * l) {
    if (l->pos > 0 && l->pos < l->len) {
        int aux = l->buf[l->pos - 1];
        l->buf[l->pos - 1] = l->buf[l->pos];
        l->buf[l->pos] = aux;
        if (l->pos != l->len - 1) {
            l->pos++;
        }
        refreshLine(l);
    }
}
