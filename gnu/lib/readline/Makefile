# $NetBSD: Makefile,v 1.2 1997/09/26 15:05:10 gwr Exp $

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

# This libraray is used only while building (not installed).
libinstall:

.include <bsd.lib.mk>

# This is for ../gdb/Makefile
echo_objdir:
	@echo ${.OBJDIR}

