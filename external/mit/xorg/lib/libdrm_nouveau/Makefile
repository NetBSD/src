#      $NetBSD: Makefile,v 1.2.2.2 2016/12/12 10:01:17 msaitoh Exp $

.include <bsd.own.mk>

LIB=	drm_nouveau
.PATH: ${X11SRCDIR.drm}
.PATH: ${X11SRCDIR.drm}/nouveau

SRCS=	nouveau.c \
	pushbuf.c \
	bufctx.c \
	abi16.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm}

LIBDPLIBS=	drm	${.CURDIR}/../libdrm

PKGCONFIG=	libdrm_nouveau
PKGDIST=	drm

.include "../libdrm/Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
