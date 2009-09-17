#	$NetBSD: Makefile,v 1.2.8.1 2009/09/17 04:24:32 snj Exp $

.include <bsd.own.mk>

LIB=	drm
.PATH:	${X11SRCDIR.${LIB}}/libdrm

SRCS=	xf86drm.c xf86drmHash.c xf86drmRandom.c xf86drmSL.c

INCS=	xf86drm.h xf86drmMode.h
INCSDIR=${X11INCDIR}/X11
.PATH:	${X11SRCDIR.${LIB}}/libdrm/intel

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

SUBDIR=	drm

PKGCONFIG=	libdrm
PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
