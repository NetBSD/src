#	$NetBSD: Makefile,v 1.5 2008/09/25 22:00:27 cube Exp $

.include <bsd.own.mk>

LIB=	XRes

XRESDIR=	${X11SRCDIR.Xres}
.PATH:		${XRESDIR}/src
.PATH:		${XRESDIR}/include/X11/extensions
SRCS=		XRes.c

INCS=	XRes.h
INCSDIR=${X11INCDIR}/X11/extensions

PKGDIST=	Xres

.include <bsd.x11.mk>
.include <bsd.lib.mk>
