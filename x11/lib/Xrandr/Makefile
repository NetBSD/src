#	$NetBSD: Makefile,v 1.7 2005/01/21 05:15:36 rtr Exp $

.include <bsd.own.mk>

LIB=	Xrandr
.PATH:	${X11SRCDIR.xc}/lib/${LIB}

SRCS=	Xrandr.c

INCS=	Xrandr.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADS}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

MAN=	Xrandr.3

LIBDPLIBS=\
	Xrender	${.CURDIR}/../Xrender \
	Xext	${.CURDIR}/../Xext \
	X11	${.CURDIR}/../X11/dynamic

.include <bsd.x11.mk>
.include <bsd.lib.mk>
