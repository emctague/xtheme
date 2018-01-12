CC=gcc
CFLAGS=-Wall -Werror
LFLAGS=
BUILDPATH=build
SRCPATH=src
OBJECTS=$(BUILDPATH)/xtheme.o
BINARY=$(BUILDPATH)/xtheme
INSTALLPREFIX=/usr/local

$(BINARY): $(BUILDPATH) $(OBJECTS)
	$(CC) $(OBJECTS) $(LFLAGS) -o $(BINARY)

$(BUILDPATH):
	mkdir $(BUILDPATH)

$(BUILDPATH)/%.o: $(SRCPATH)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(BINARY)
	install $(BINARY) $(INSTALLPREFIX)/bin

