# $NetBSD: Makefile,v 1.1.1.1 1997/09/26 02:53:21 gwr Exp $

LIB=	readline

NOLINT=
NOMAN=
NOPIC=
NOPROFILE=

TOP=	$(.CURDIR)/../..
dist=	$(TOP)/dist
srcdir= $(dist)/readline

# Note: sysdep.h is found here.
CFLAGS+= -I$(.CURDIR) -I$(srcdir) -I$(dist)/include
CFLAGS+= -DHAVE_CONFIG_H

.PATH:	$(srcdir)

# From OBJECTS
OBJS=     readline.o vi_mode.o funmap.o keymaps.o parens.o search.o \
	  rltty.o complete.o bind.o isearch.o display.o signals.o \
	  util.o kill.o undo.o macro.o input.o callback.o terminal.o \
	  nls.o history.o histexpand.o histfile.o histsearch.o \
	  shell.o tilde.o xmalloc.o

all: lib${LIB}.a

lib${LIB}.a: ${OBJS}
	-rm -f lib${LIB}.a
	${AR} cq lib${LIB}.a ${OBJS}
	${RANLIB} lib${LIB}.a

clean:
	-rm -f lib$(LIB).a
	-rm -f $(OBJS)

# This is for ../gdb/Makefile
echo_objdir:
	@echo ${.OBJDIR}

depend:

install:

