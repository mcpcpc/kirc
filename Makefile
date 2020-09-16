CFLAGS += -std=c89 -Wall -Wextra -pedantic -Wold-style-declaration
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: kirc

kirc: kirc.o Makefile
	$(CC) $(CFLAGS) -o $@ $@.o $(LDFLAGS)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

install: all
	install -Dm755 kirc $(DESTDIR)$(BINDIR)/kirc

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/kirc

clean:
	rm -f kirc *.o

.PHONY: all install uninstall clean
