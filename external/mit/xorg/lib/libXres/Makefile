#	$NetBSD: Makefile,v 1.3 2008/09/25 17:12:11 cube Exp $

.include <bsd.own.mk>

LIB=	XRes

XRESDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XRESDIR}/src
.PATH:		${XRESDIR}/include/X11/extensions
SRCS=		XRes.c

INCS=	XRes.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
