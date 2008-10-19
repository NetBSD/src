#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:11 haad Exp $

.include <bsd.own.mk>

LIB=	drm
.PATH:	${X11SRCDIR.${LIB}}/libdrm

SRCS=	xf86drm.c xf86drmHash.c xf86drmRandom.c xf86drmSL.c

INCS=	xf86drm.h xf86mm.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

SUBDIR=	drm

PKGCONFIG=	libdrm
PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
