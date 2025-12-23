.POSIX:
.SUFFIXES:

include config.mk

CFLAGS += -std=c99 -pedantic -Wall -Wextra
CFLAGS += -Wformat-security -Wwrite-strings
CFLAGS += -Wno-unused-parameter
CFLAGS += -g -Iinclude -Isrc

BIN = kirc
SRC = src
BUILD = build

# Discover all source files
SRCS = $(wildcard $(SRC)/*.c)

# Create matching build/*.o paths
OBJS = $(SRCS:$(SRC)/%.c=$(BUILD)/%.o)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD):
	mkdir -p $@

# Pattern rule: build/xyz.o ← src/xyz.c
$(BUILD)/%.o: $(SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(BIN) $(BUILD)/*

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f $(BIN) $(DESTDIR)$(BINDIR)
	chmod 755 $(DESTDIR)$(BINDIR)/$(BIN)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp -f $(BIN).1 $(DESTDIR)$(MANDIR)/man1
	chmod 644 $(DESTDIR)$(MANDIR)/man1/$(BIN).1

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(BIN)
	rm -f $(DESTDIR)$(MANDIR)/man1/$(BIN).1

.PHONY: all clean install uninstall
