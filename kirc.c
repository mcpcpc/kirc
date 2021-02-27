#include <unistd.h>
#include "kirc.h"

static int die(char * errstr) {
    size_t n = 0;
    char * p = errstr;
    while ((* (p++)) != 0) {
        ++n;
    }
    ssize_t o = write(STDERR_FILENO, errstr, n);
    int ret = 1;
    if (o < 0) {
        ret = -1;
    }
    return ret;
}

static void disableRawMode() {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig) == -1) {
		die("tcsetattr() failed\n");
	}
}

static int enableRawMode() {
    int ret  = 0;
	if (tcgetattr(STDIN_FILENO, &E.orig) == -1) {
	    ret = die("tcgetattr() failed\n");
	}
	if (ret == 0) {
	    atexit(disableRawMode);
        	struct termios raw = E.orig;
        	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
        	raw.c_oflag &= ~(OPOST);
        	raw.c_cflag |= (CS8);
        	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
        	raw.c_cc[VMIN] = 0;
        	raw.c_cc[VTIME] = 1;
	}
	if ((ret == 0) && (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)) {
	    ret = die("tcsetattr() failed\n");
	}
	return ret;
}

int cursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1) {
			break;
		}
		if (buf[i] == 'R') {
			break;
		}
		i++;
	}
	buf[i] = '\0';
	if (buf[0] != '\x1b' || buf[1] != '[') {
		return -1;
	}
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
		return -1;
	}
	return 0;
}

static int windowSize(int *rows, int *cols) {
	struct winsize ws;
	int ret = 0;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
		    return -1;
		}
		ret = getCursorPosition(rows, cols);
	} else {
		*cols = ws.ws_col;
		*rows = ws.ws_row;
	}
	return ret;
}

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

static void moveLeft(struct State * l) {
    if (l->pos > 0) {
        l->pos--;
        refreshLine(l);
    }
}

static void moveRight(struct State * l) {
    if (l->pos != l->len) {
        l->pos++;
        refreshLine(l);
    }
}

static void moveHome(struct State * l) {
    if (l->pos != 0) {
        l->pos = 0;
        refreshLine(l);
    }
}

static void moveEnd(struct State * l) {
    if (l->pos != l->len) {
        l->pos = l->len;
        refreshLine(l);
    }
}

static void delete(struct State * l) {
    if (l->len > 0 && l->pos < l->len) {
        memmove(l->buf + l->pos, l->buf + l->pos + 1, l->len - l->pos - 1);
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void backspace(struct State * l) {
    if (l->pos > 0 && l->len > 0) {
        memmove(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
        l->pos--;
        l->len--;
        l->buf[l->len] = '\0';
        refreshLine(l);
    }
}

static void deletePrevWord(struct State * l) {
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

static void deleteWholeLine(struct State * l) {
    l->buf[0] = '\0';
    l->pos = l->len = 0;
    refreshLine(l);
}

static void deleteLineToEnd(struct State * l) {
    l->buf[l->pos] = '\0';
    l->len = l->pos;
    refreshLine(l);
}

static void swapCharWithPrev(struct State * l) {
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

static void refreshScreen() {
	scroll();
	struct abuf ab = ABUF_INIT;
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);
	drawRows(&ab);
	drawMessageBar(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
	abAppend(&ab, buf, strlen(buf));
	abAppend(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

