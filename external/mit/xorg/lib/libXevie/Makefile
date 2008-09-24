#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:03 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xevie

XEVIEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XEVIEDIR}/src
.PATH:		${XEVIEDIR}/include/X11/extensions
SRCS=		Xevie.c

INCS=	Xevie.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
