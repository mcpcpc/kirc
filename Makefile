.POSIX:

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

all: kirc

kirc: kirc.o Makefile
	$(CC) -o kirc kirc.o $(LDFLAGS)

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f kirc $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/kirc

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/kirc

clean:
	rm -f kirc *.o

.PHONY: all install uninstall clean
