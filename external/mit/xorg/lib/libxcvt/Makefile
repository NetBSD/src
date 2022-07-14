#	$NetBSD: Makefile,v 1.1 2022/07/14 03:48:34 mrg Exp $

.include <bsd.own.mk>

LIB=	xcvt
.PATH:	${X11SRCDIR.${LIB}}/lib
.PATH:	${X11SRCDIR.${LIB}}/include/libxcvt

SRCS=	libxcvt.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libxcvt

INCS=	libxcvt.h libxcvt_mode.h
INCSDIR=${X11INCDIR}/libxcvt

PKGCONFIG=	libxcvt

.include <bsd.x11.mk>
.include <bsd.lib.mk>
