#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:09 haad Exp $

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
