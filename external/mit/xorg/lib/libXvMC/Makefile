#	$NetBSD: Makefile,v 1.4.26.1 2020/04/08 14:07:02 martin Exp $

.include <bsd.own.mk>

LIB=	XvMC
.PATH:	${X11SRCDIR.${LIB}}/src
.PATH:	${X11SRCDIR.${LIB}}/include/X11/extensions

SRCS=	XvMC.c

INCS=	XvMClib.h vldXvMC.h
INCSDIR=${X11INCDIR}/X11/extensions

CPPFLAGS+=	${X11FLAGS.THREADLIB}
CPPFLAGS+=	-DHAVE__XEATDATAWORDS

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
