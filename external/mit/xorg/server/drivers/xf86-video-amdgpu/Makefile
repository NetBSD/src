#	$NetBSD: Makefile,v 1.1.2.2 2016/09/14 03:04:16 pgoyette Exp $

.include <bsd.own.mk>

DRIVER=		xf86-video-amdgpu
DRIVER_NAME=	amdgpu_drv

LIBDPLIBS+=	drm_amdgpu ${.CURDIR}/../../../lib/libdrm_amdgpu

SRCS.KMS=	amdgpu_bo_helper.c amdgpu_dri2.c amdgpu_dri3.c \
		amdgpu_drm_queue.c amdgpu_kms.c amdgpu_present.c \
		amdgpu_sync.c drmmode_display.c
SRCS.GLAMOR=	amdgpu_glamor.c amdgpu_glamor_wrappers.c amdgpu_pixmap.c

SRCS=		amdgpu_video.c amdgpu_misc.c amdgpu_probe.c \
		$(SRCS.KMS) $(SRCS.GLAMOR)

MAN=		amdgpu.4

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/dri \
		-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.${DRIVER}}/../include

# XXX
COPTS+=	-Wno-error

.include "../Makefile.xf86-driver"
