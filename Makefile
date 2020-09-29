.POSIX:

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
MANPREFIX = $(PREFIX)/share/man

all: kirc

kirc: kirc.o Makefile
	$(CC) -o kirc kirc.o $(LDFLAGS)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -f kirc $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/kirc
	version=$$(sed -n '/#define VERSION/{s/^[^"]*"//;s/".*//;p;q}' kirc.c); \
		sed "s/VERSION/$$version/g" kirc.1 > $(DESTDIR)$(MANPREFIX)/man1/kirc.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/kirc.1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/kirc
	rm -f $(DESTDIR)$(MANPREFIX)/man1/kirc.1

clean:
	rm -f kirc *.o

.PHONY: all install uninstall clean
