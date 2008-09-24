#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:05 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xv
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	Xv.c

CPPFLAGS+=	${X11FLAGS.THREADLIB}

INCS=	Xvlib.h
INCSDIR=${X11INCDIR}/X11/extensions

LIBDPLIBS=\
	Xext	${.CURDIR}/../libXext \
	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
