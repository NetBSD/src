#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:22 mrg Exp $

.include <bsd.own.mk>

PROG=	resize
SRCS=	resize.c xstrings.c

CPPFLAGS+=-I${X11SRCDIR.xterm}

.PATH:	${X11SRCDIR.xterm}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
