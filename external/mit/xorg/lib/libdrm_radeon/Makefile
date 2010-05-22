#      $NetBSD: Makefile,v 1.1 2010/05/22 22:48:19 mrg Exp $

.include <bsd.own.mk>

LIB=	drm_radeon
.PATH: ${X11SRCDIR.drm}/radeon

SRCS=	radeon_bo.c \
	radeon_bo_gem.c \
	radeon_cs.c \
	radeon_cs_gem.c \
	radeon_cs_space.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm}

# XXX not yet
#PKGCONFIG=	libdrm_radeon

COPTS.radeon_cs_gem.c+=	-Wno-error	# XXX

.include <bsd.x11.mk>
.include <bsd.lib.mk>
