#	$NetBSD: Makefile,v 1.2.2.3 2008/09/24 16:35:05 wrstuden Exp $

.include <bsd.own.mk>

LIB=	Xres

XRESDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XRESDIR}/src
.PATH:		${XRESDIR}/include/X11/extensions
SRCS=		XRes.c

INCS=	XRes.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
