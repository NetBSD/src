#	$NetBSD: Makefile,v 1.4 2008/09/25 21:55:35 cube Exp $

.include <bsd.own.mk>

LIB=	XRes

XRESDIR=	${X11SRCDIR.Xres}
.PATH:		${XRESDIR}/src
.PATH:		${XRESDIR}/include/X11/extensions
SRCS=		XRes.c

INCS=	XRes.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGCONFIG=	xres
PKGDIST.xres=	${XRESDIR}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
