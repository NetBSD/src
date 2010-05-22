#	$NetBSD: Makefile,v 1.6 2010/05/22 22:43:57 mrg Exp $

.include <bsd.own.mk>

LIB=	drm
.PATH:	${X11SRCDIR.${LIB}}

SRCS=	xf86drm.c xf86drmHash.c xf86drmRandom.c xf86drmSL.c

INCS=	xf86drm.h xf86drmMode.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm

SUBDIR=	drm

PKGCONFIG=	libdrm
PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
