#	$NetBSD: Makefile,v 1.6.4.2 2008/10/19 22:41:11 haad Exp $

.include <bsd.own.mk>

LIB=	XRes

XRESDIR=	${X11SRCDIR.Xres}
.PATH:		${XRESDIR}/src
.PATH:		${XRESDIR}/man
.PATH:		${XRESDIR}/include/X11/extensions
SRCS=		XRes.c

INCS=	XRes.h
INCSDIR=${X11INCDIR}/X11/extensions

MAN=	XRes.3
MLINKS=	XRes.3 XResQueryExtension.3 \
	XRes.3 XResQueryVersion.3 \
	XRes.3 XResQueryClients.3 \
	XRes.3 XResQueryClientResources.3 \
	XRes.3 XResQueryClientPixmapBytes.3

PKGDIST=	Xres

.include <bsd.x11.mk>
.include <bsd.lib.mk>
