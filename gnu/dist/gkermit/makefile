# makefile for gkermit - works with make or gmake.
#
# Author:
#   Frank da Cruz
#   The Kermit Project, Columbia University
#   http://www.columbia.edu/kermit/
#   kermit@columbia.edu
#   December 1999
#
# Main build targets:
#   posix:     Build for any POSIX-based platform (default).
#   sysv:      Build for any AT&T UNIX System V based platform.
#   bsd:       Build for any UNIX V7 or 4.3 (or earlier) BSD based platform.
#
# Special build targets:
#   sysvx      Like sysv but uses getchar()/putchar().
#   stty       Uses system("stty blah") instead of API calls.
#   bsd211     For 2.11BSD on the PDP-11 - no nested makes.
#
# Other targets:
#   clean:     Remove object files
#   install:   Install gkermit
#   uninstall: Uninstall gkermit
#
# Default compiler is cc.  To force gcc use:
#   make "CC=gcc" [ <target> ]
#
# See README and COPYING for further information.

# Sample installation values - change or override as needed.

BINDIR = /usr/local/bin
MANDIR = /usr/man/manl
TEXTDIR = /usr/local/doc
INFODIR = /usr/local/info
MANEXT = l

# Default compiler and flags

CC=cc
CFLAGS= -DPOSIX -O $(KFLAGS)

# Object files

OBJECTS= gproto.o gkermit.o gunixio.o gcmdline.o

# Targets and dependencies

all:		gwart gkermit

gwart.o:	gwart.c
		$(CC) $(CFLAGS) -c gwart.c

gwart:		gwart.o
		$(CC) -o gwart gwart.o

.c.o:
		$(CC) $(CFLAGS) -c $<

gproto.c:	gproto.w gkermit.h
		./gwart gproto.w gproto.c

gkermit.o:	gkermit.c gkermit.h

gunixio.o:	gunixio.c gkermit.h

gcmdline.o:	gcmdline.c gkermit.h

gkermit:	gproto.o gkermit.o gunixio.o gcmdline.o
		$(CC) -o gkermit $(OBJECTS)

bsd:		gwart
		$(MAKE) "CC=$(CC)" "CFLAGS=-DBSD -O $(KFLAGS)" gkermit

sysv:		gwart
		$(MAKE) "CC=$(CC)" "CFLAGS=-DSYSV -O $(KFLAGS)" gkermit

posix:		gwart
		$(MAKE) "CC=$(CC)" "CFLAGS=-DPOSIX -O $(KFLAGS)" gkermit

sysvx:		gwart
		$(MAKE) "CC=$(CC)" \
		"CFLAGS=-DSYSV -DUSE_GETCHAR -O $(KFLAGS)" gkermit

stty:		gwart
		$(MAKE) "CC=$(CC)" "CFLAGS=$(KFLAGS)" gkermit

bsd211:		gwart
		./gwart gproto.w gproto.c
		cc -DBSD $(KFLAGS) -c gkermit.c
		cc -DBSD $(KFLAGS) -c gproto.c
		cc -DBSD $(KFLAGS) -c gcmdline.c
		cc -DBSD $(KFLAGS) -c gunixio.c
		cc -o gkermit $(OBJECTS)

clean:
		rm -f $(OBJECTS) gproto.o gproto.c gwart.o gwart

install:
		@if test -f ./gkermit; then \
		    echo "Installing gkermit..." ; \
		else \
		    echo "Please build the gkermit binary first." ; \
		    exit ; \
		fi
		@echo Copying gkermit to $(BINDIR)...
		@cp gkermit $(BINDIR)/gkermit
		@chmod 755 $(BINDIR)/gkermit
		@ls -lg $(BINDIR)/gkermit
		@if test -d $(TEXTDIR); then \
		    echo "$(TEXTDIR) exists..." ; \
		else \
		    echo "Creating $(TEXTDIR)/..." ; \
		    mkdir $(TEXTDIR) ; \
		    chmod 755 $(TEXTDIR) ; \
		fi
		@echo Copying README to $(TEXTDIR)/gkermit.txt...
		@cp README $(TEXTDIR)/gkermit.txt
		@chmod 644 $(TEXTDIR)/gkermit.txt
		@ls -lg $(TEXTDIR)/gkermit.txt
		@echo Installing man page in $(MANDIR)/gkermit.$(MANEXT)...
		@cp gkermit.nr $(MANDIR)/gkermit.$(MANEXT)
		@chmod 644 $(MANDIR)/gkermit.$(MANEXT)
		@ls -lg $(MANDIR)/gkermit.$(MANEXT)

uninstall:
		@echo Uninstalling gkermit...
		rm -f $(BINDIR)/gkermit \
		$(TEXTDIR)/gkermit.txt \
		$(MANDIR)gkermit.$(MANEXT)

.PHONY:		clean install uninstall

# (end)
