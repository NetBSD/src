#	$NetBSD: Makefile,v 1.2 2003/09/13 20:18:37 lukem Exp $

.include <bsd.own.mk>

PROG=	resize
SRCS=	resize.c xstrings.c

CPPFLAGS+=-I${X11SRCDIR.xc}/programs/xterm

.PATH:	${X11SRCDIR.xc}/programs/xterm

.include <bsd.x11.mk>
.include <bsd.prog.mk>
