#	$NetBSD: Makefile,v 1.6 2004/08/05 01:28:48 lukem Exp $

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
	Xrender	${.CURDIR}/../Xrender \
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
