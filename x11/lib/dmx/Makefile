#	$NetBSD: Makefile,v 1.1 2005/03/22 04:18:48 rtr Exp $

.include <bsd.own.mk>

LIB=	dmx
.PATH:	${X11SRCDIR.xc}/lib/dmx

SRCS=	dmx.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/include/extensions
CPPFLAGS+=	${X11FLAGS.THREADS}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
