#	$NetBSD: Makefile,v 1.1.2.2 2005/04/06 21:22:45 tron Exp $

.include <bsd.own.mk>

LIB=	dmx
.PATH:	${X11SRCDIR.xc}/lib/dmx

SRCS=	dmx.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
