#	$NetBSD: Makefile,v 1.5 2004/08/04 08:37:04 lukem Exp $

.include <bsd.own.mk>

LIB=	Xrandr
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Xrandr.c

INCS=	Xrandr.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

MAN=	Xrandr.3

LIBDPLIBS=\
	Xrender ../Xrender \
	Xext ../Xext \
	X11 ../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
