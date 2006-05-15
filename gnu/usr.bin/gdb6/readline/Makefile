#	$NetBSD: Makefile,v 1.1 2006/05/15 16:31:37 nathanw Exp $

.include <bsd.own.mk>

MKPRIVATELIB=	yes

TOP=		${NETBSDSRCDIR}/gnu
DIST=		${TOP}/dist/gdb6

LIB=readline

NOGCCERROR=	yes

CPPFLAGS+=-I${.CURDIR} -I${DIST} -DHAVE_CONFIG_H

SRCS=bind.c callback.c compat.c complete.c display.c funmap.c		\
histexpand.c histfile.c history.c histsearch.c input.c isearch.c	\
keymaps.c kill.c macro.c mbutil.c misc.c nls.c parens.c readline.c	\
rltty.c search.c shell.c signals.c terminal.c text.c tilde.c undo.c	\
util.c vi_mode.c xmalloc.c

.include "../../Makefile.inc"
.include <bsd.lib.mk>


.PATH: ${DIST}/readline
