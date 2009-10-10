#	$NetBSD: Makefile,v 1.2.8.2 2009/10/10 19:53:22 sborrill Exp $

.include <bsd.own.mk>

LIB=	drm
.PATH:	${X11SRCDIR.${LIB}}/libdrm

SRCS=	xf86drm.c xf86drmHash.c xf86drmRandom.c xf86drmSL.c

INCS=	xf86drm.h xf86drmMode.h
INCSDIR=${X11INCDIR}
.PATH:	${X11SRCDIR.${LIB}}/libdrm/intel

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/drm

SUBDIR=	drm

PKGCONFIG=	libdrm
PKGDIST=	${LIB}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
.include <bsd.subdir.mk>
