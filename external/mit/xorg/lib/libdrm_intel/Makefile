#      $NetBSD: Makefile,v 1.1 2009/06/09 03:05:37 mrg Exp $

.include <bsd.own.mk>

LIB=	drm_intel
.PATH: ${X11SRCDIR.drm}/libdrm/intel

SRCS=	intel_bufmgr.c intel_bufmgr_fake.c intel_bufmgr_gem.c mm.c

INCS=	intel_bufmgr.h
INCSDIR=${X11INCDIR}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm}/libdrm

.include <bsd.x11.mk>
.include <bsd.lib.mk>
