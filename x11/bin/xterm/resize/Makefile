#	$NetBSD: Makefile,v 1.1.1.1 2003/09/11 18:37:08 lukem Exp $

.include <bsd.own.mk>

PROG=	resize
SRCS=	resize.c xstrings.c

CPPFLAGS+=-I${X11SRCDIR.xc}/programs/xterm

.PATH:	${X11SRCDIR.xc}/programs/xterm

.include <bsd.prog.mk>
.include <bsd.x11.mk>
