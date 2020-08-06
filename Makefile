CFLAGS += -std=c99 -Wall -Wextra -pedantic -Wold-style-declaration
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: kirc

kfc: kirc.c Makefile
	$(CC) -O3 $(CFLAGS) -o $@ $< -lX11 $(LDFLAGS)

install: all
	install -Dm755 kirc $(DESTDIR)$(BINDIR)/kirc

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/kirc

clean:
	rm -f kirc *.o

.PHONY: all install uninstall clean
