#	$NetBSD: Makefile,v 1.4 2004/08/04 08:37:05 lukem Exp $

.include <bsd.own.mk>

LIB=	Xtst
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	XTest.c XRecord.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

LIBDPLIBS=\
	Xext ../Xext \
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
