#      $NetBSD: Makefile,v 1.7.4.1 2016/07/13 19:34:23 snj Exp $

.include <bsd.own.mk>

LIB=	drm_intel
.PATH: ${X11SRCDIR.drm}/intel

SRCS=	intel_bufmgr.c intel_bufmgr_fake.c intel_bufmgr_gem.c \
	intel_decode.c mm.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm}

LIBDPLIBS=	drm	${.CURDIR}/../libdrm

PKGCONFIG=	libdrm_intel
PKGDIST=	drm

CWARNFLAGS.clang+=	-Wno-pointer-sign

.include <bsd.x11.mk>
.include <bsd.lib.mk>
