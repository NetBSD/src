#	$NetBSD: Makefile,v 1.2 2008/09/13 04:32:30 cube Exp $

.include <bsd.own.mk>

LIB=	Xdamage

XDAMAGEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XDAMAGEDIR}/src
.PATH:		${XDAMAGEDIR}/include/X11/extensions
SRCS=		Xdamage.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/extensions

INCS=	Xdamage.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
