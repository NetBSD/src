# $NetBSD: Makefile,v 1.4 1997/10/25 09:27:39 lukem Exp $

LIB=	readline

NOLINT=
NOMAN=
NOPIC=
NOPROFILE=

TOP=	$(.CURDIR)/../..
dist=	$(TOP)/dist
srcdir= $(dist)/readline

# Note: sysdep.h is found here.
CPPFLAGS+= -I$(.CURDIR) -I$(srcdir) -I$(dist)/include
CPPFLAGS+= -DHAVE_CONFIG_H

.PATH:	$(srcdir)

# From OBJECTS
SRCS=     readline.c vi_mode.c funmap.c keymaps.c parens.c search.c \
	  rltty.c complete.c bind.c isearch.c display.c signals.c \
	  util.c kill.c undo.c macro.c input.c callback.c terminal.c \
	  nls.c history.c histexpand.c histfile.c histsearch.c \
	  shell.c tilde.c xmalloc.c

# This libraray is used only while building (not installed).
libinstall:

.include <bsd.lib.mk>

# This is for ../gdb/Makefile
echo_objdir:
	@echo ${.OBJDIR}

