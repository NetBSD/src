#	$NetBSD: Makefile,v 1.3 2003/09/14 09:50:16 martin Exp $

.include <bsd.own.mk>

MKPIC=no
MKPROFILE=no  
MKLINT=no	# lint misses important ifdef magic on 64 bit archs

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

LIB=readline

CPPFLAGS+=-I${.CURDIR} -I${DIST}/readline -DHAVE_CONFIG_H

SRCS=bind.c callback.c complete.c display.c funmap.c histexpand.c \
histfile.c history.c histsearch.c input.c isearch.c keymaps.c \
kill.c macro.c nls.c parens.c readline.c rltty.c search.c shell.c \
signals.c terminal.c tilde.c undo.c util.c vi_mode.c xmalloc.c

libinstall::

.include "../../Makefile.inc"
.include <bsd.lib.mk>


.PATH: ${DIST}/readline
