#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:04 haad Exp $

.include <bsd.own.mk>

PROG=	resize
SRCS=	resize.c xstrings.c

CPPFLAGS+=-I${X11SRCDIR.xterm}

.PATH:	${X11SRCDIR.xterm}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
