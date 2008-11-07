#	$NetBSD: Makefile,v 1.2.8.1 2008/11/07 23:23:35 snj Exp $

.include <bsd.own.mk>

LIB=	Xdamage

XDAMAGEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XDAMAGEDIR}/src
.PATH:		${XDAMAGEDIR}/include/X11/extensions
SRCS=		Xdamage.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

INCS=	Xdamage.h
INCSDIR=${X11INCDIR}/X11/extensions

LIBDPLIBS=	X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
