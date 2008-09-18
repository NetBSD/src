#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:32 wrstuden Exp $

.include <bsd.own.mk>

PROG=	resize
SRCS=	resize.c xstrings.c

CPPFLAGS+=-I${X11SRCDIR.xterm}

.PATH:	${X11SRCDIR.xterm}

.include <bsd.x11.mk>
.include <bsd.prog.mk>
