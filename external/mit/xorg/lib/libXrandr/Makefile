#	$NetBSD: Makefile,v 1.3 2011/08/11 23:15:39 joerg Exp $

.include <bsd.own.mk>

LIB=	Xrandr
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions
.PATH:	${X11SRCDIR.${LIB}}/man

SRCS=	Xrandr.c XrrConfig.c XrrCrtc.c XrrMode.c XrrOutput.c \
	XrrProperty.c XrrScreen.c

INCS=	Xrandr.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

MAN=	Xrandr.3

LIBDPLIBS=\
	Xrender	${.CURDIR}/../libXrender \
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

CWARNFLAGS.clang+=	-Wno-pointer-sign

.include <bsd.x11.mk>
.include <bsd.lib.mk>
