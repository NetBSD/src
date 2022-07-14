#	$NetBSD: Makefile,v 1.2 2022/07/14 07:41:50 mrg Exp $

.include <bsd.own.mk>

LIB=	xcvt

SRCS=	libxcvt.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libxcvt

INCS=	libxcvt.h libxcvt_mode.h
INCSDIR=${X11INCDIR}/libxcvt

PKGCONFIG=	libxcvt

.include <bsd.x11.mk>
.include <bsd.lib.mk>

.PATH:	${X11SRCDIR.${LIB}}/lib
.PATH:	${X11SRCDIR.${LIB}}/include/libxcvt
