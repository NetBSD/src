#	$NetBSD: Makefile,v 1.1 2003/08/17 18:03:29 christos Exp $

.include <bsd.own.mk>

MKPIC=no
MKPROFILE=no  

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb

LIB=readline

CPPFLAGS+=-I${.CURDIR} -I${DIST}/readline -DHAVE_CONFIG_H

SRCS=bind.c callback.c complete.c display.c funmap.c histexpand.c \
histfile.c history.c histsearch.c input.c isearch.c keymaps.c \
kill.c macro.c nls.c parens.c readline.c rltty.c search.c shell.c \
signals.c terminal.c tilde.c undo.c util.c vi_mode.c xmalloc.c

.include "../../Makefile.inc"
.include <bsd.lib.mk>


.PATH: ${DIST}/readline
