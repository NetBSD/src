#	$NetBSD: Makefile,v 1.6.6.1 2008/11/07 23:23:36 snj Exp $

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

LIBDPLIBS=	Xext	${.CURDIR}/../libXext \
		X11	${.CURDIR}/../libX11/dynamic

PKGDIST=	Xres

.include <bsd.x11.mk>
.include <bsd.lib.mk>
