.POSIX:
ALL_WARNING = -Wall -Wextra -pedantic -std=c99
PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man

kirc: kirc.c kirc.h
	$(CC) $(CFLAGS) -D_FILE_OFFSET_BITS=64 $(LDFLAGS) ${ALL_WARNING} kirc.c -o kirc
all: kirc

install: kirc
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp -f kirc $(DESTDIR)$(BINDIR)
	cp -f kirc.1 $(DESTDIR)$(MANDIR)/man1
	chmod 755 $(DESTDIR)$(BINDIR)/kirc
	chmod 644 $(DESTDIR)$(MANDIR)/man1/kirc.1
clean:
	rm -f kirc
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/kirc
	rm -f $(DESTDIR)$(MANDIR)/man1/kirc.1
.PHONY: all install uninstall clean
