#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
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

static int cursorPosition(int *rows, int *cols) {
	char buf[32];
	unsigned int i = 0;
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
		return -1;
	}
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
		ret = cursorPosition(rows, cols);
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

static void memcpy_c(void *dest, void *src, size_t n) {
	char *csrc = (char *)src;
	char *cdest = (char *)dest;
	for (size_t i=0; i<n; i++) {
		cdest[i] = csrc[i];
	}
}

static void *memmove_c(void *s1, const void *s2, size_t n) {
	char *pDest = (char *)s1;
	const char *pSrc =( const char*)s2;
	char *tmp  = (char *)malloc(sizeof(char ) * n);
	if(NULL == tmp) {
		return NULL;
	}
	else {
		size_t i = 0;
		for(i =0; i < n ; ++i) {
			*(tmp + i) = *(pSrc + i);
		}
		for(i =0 ; i < n ; ++i) {
			*(pDest + i) = *(tmp + i);
		}
		free(tmp);
	}
	return s1;
}

static void *memset_c(void *s, int c, size_t n) {
	unsigned char *p = s;
	while (n > 0) {
		*p = c;
		p++;
		n--;
	}
	return s;
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
	memcpy_c(new + ab->len, s, len);
	ab->b = new;
	ab->len += len;
}

static void abFree(struct abuf * ab) {
	free(ab->b);
}

static int insert(struct State * l, char c) {
	if (l->len < l->buflen) {
		if (l->len == l->pos) {
			l->buf[l->pos] = c;
			l->pos++;
			l->len++;
			l->buf[l->len] = '\0';
			if (l->plen + l->len < l->cols) {
				char d = c;
				if (write(STDOUT_FILENO, &d, 1) == -1)
					return -1;
			} else {
				refreshLine(l);
			}
		} else {
			memmove_c(l->buf+l->pos+1,l->buf+l->pos,l->len-l->pos);
			l->buf[l->pos] = c;
			l->len++;
			l->pos++;
			l->buf[l->len] = '\0';
			refreshLine(l);
		}
	}
	return 0;
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
		memmove_c(l->buf + l->pos, l->buf + l->pos + 1, l->len - l->pos - 1);
		l->len--;
		l->buf[l->len] = '\0';
		refreshLine(l);
	}
}

static void backspace(struct State * l) {
	if (l->pos > 0 && l->len > 0) {
		memmove_c(l->buf+l->pos-1,l->buf+l->pos,l->len-l->pos);
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
	memmove_c(l->buf+l->pos,l->buf+old_pos,l->len-old_pos+1);
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
	struct abuf ab;
	abInit(&ab);
	abAppend(&ab, "\x1b[?25l", 6);
	abAppend(&ab, "\x1b[H", 3);
	drawRows(&ab);
	drawMessageBar(&ab);
	char buf[32];
	snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
	abAppend(&ab, buf, strlen_c(buf));
	abAppend(&ab, "\x1b[?25h", 6);
	write(STDOUT_FILENO, ab.b, ab.len);
	abFree(&ab);
}

static int edit(struct State * l) {
	char	c, seq[3];
	ssize_t nread = read(STDIN_FILENO, &c, 1);

	if (nread <= 0)
		return 1;

	switch(c) {
	case 13:					  return 1;  /* enter */
	case 3: errno = EAGAIN;	   return -1; /* ctrl-c */
	case 127:								/* backspace */
	case 8:  backspace(l);		break; /* ctrl-h */
	case 2:  moveLeft(l);		 break; /* ctrl-b */
	case 6:  moveRight(l);		break; /* ctrl-f */
	case 1:  moveHome(l);		 break; /* Ctrl+a */
	case 5:  moveEnd(l);		  break; /* ctrl+e */
	case 23: deletePrevWord(l);   break; /* ctrl+w */
	case 21: deleteWholeLine(l);  break; /* Ctrl+u */
	case 11: deleteLineToEnd(l);  break; /* Ctrl+k */
	case 20: swapCharWithPrev(l); break; /* ctrl-t */
	case 4:								  /* ctrl-d */
		if (l->len > 0) {
			delete(l);
		} else {
			return -1;
		}
		break;
	case 27:	/* escape sequence */
		if (read(STDIN_FILENO, seq, 1) == -1) break;
		if (read(STDIN_FILENO, seq + 1, 1) == -1) break;
		if (seq[0] == '[') { /* ESC [ sequences. */
			if (seq[1] >= '0' && seq[1] <= '9') {
				/* Extended escape, read additional byte. */
				if (read(STDIN_FILENO, seq + 2, 1) == -1) break;
				if (seq[2] == '~') {
					if (seq[1] == 3) delete(l);	/* Delete key. */
				}
			} else {
				switch(seq[1]) {
				case 'C': moveRight(l); break; /* Right */
				case 'D': moveLeft(l);  break; /* Left */
				case 'H': moveHome(l);  break; /* Home */
				case 'F': moveEnd(l);   break; /* End*/
				}
			}
		}
		else if (seq[0] == 'O') { /* ESC O sequences. */
			switch(seq[1]) {
			case 'H': moveHome(l); break; /* Home */
			case 'F': moveEnd(l);  break; /* End*/
			}
		}
		break;
	default: if (insert(l, c)) return -1; break;
	}
	return 0;
}

static void stateReset(struct State * l) {
	l->plen = strlen_c(l->prompt);
	l->oldpos = 0;
	l->pos = 0;
	l->len = 0;
	l->buf[0] = '\0';
	l->buflen--;
}

static int tcpOpen(const char *host, const char *service) {
	struct addrinfo hints, *res = NULL, *rp;
	int fd = -1, e;
	memset_c(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICSERV;
	hints.ai_socktype = SOCK_STREAM;
	if ((e = getaddrinfo(host, service, &hints, &res))) {
		die("getaddrinfo(): error");
	} else {
		for (rp = res; rp; rp = rp->ai_next) {
			fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
			if (fd == -1) {
				continue;
			}
			if (connect(fd, rp->ai_addr, rp->ai_addrlen) == -1) {
				close(fd);
				fd = -1;
				continue;
			}
			break;
		}
		if (fd == -1) {
			die("socket(): could not connect\n");
		} else {
			freeaddrinfo(res);
		}
	}
	return fd;
}

int main(int argc, char *argv[]) {
	int ret = 0;
	int sock = tcpOpen("irc.freenode.net", "6667");
	struct pollfd fds[2];
	fds[0].fd = STDIN_FILENO;
	fds[1].fd = sock;
	fds[0].events = POLLIN;
	fds[1].events = POLLIN;
	while (ret == 0) {
		if (poll(fds, 2, -1) != -1) {
				if (fds[0].revents & POLLIN) {
				}
				if (fds[1].revents & POLLIN) {
				}
		}
	}
	return ret;
}
