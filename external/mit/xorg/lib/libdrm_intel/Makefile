#      $NetBSD: Makefile,v 1.4 2010/05/22 22:43:57 mrg Exp $

.include <bsd.own.mk>

LIB=	drm_intel
.PATH: ${X11SRCDIR.drm}/intel

SRCS=	intel_bufmgr.c intel_bufmgr_fake.c intel_bufmgr_gem.c mm.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm}

# XXX not yet
#PKGCONFIG=	libdrm_intel

.include <bsd.x11.mk>
.include <bsd.lib.mk>
