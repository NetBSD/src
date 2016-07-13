#	$NetBSD: Makefile,v 1.8.4.1 2016/07/13 19:34:23 snj Exp $

.include <bsd.own.mk>

LIB=	drm
.PATH:	${X11SRCDIR.${LIB}}

SRCS=	xf86drm.c xf86drmHash.c xf86drmMode.c xf86drmRandom.c xf86drmSL.c

INCS=	xf86drm.h xf86drmMode.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm

SUBDIR=	drm kms

PKGCONFIG=	libdrm
PKGDIST=	${LIB}

CWARNFLAGS.clang+=	-Wno-conversion -Wno-tautological-compare

.include "Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
