#      $NetBSD: Makefile,v 1.2.2.2 2016/09/14 03:04:15 pgoyette Exp $

.include <bsd.own.mk>

LIB=	drm_amdgpu
.PATH: ${X11SRCDIR.drm}
.PATH: ${X11SRCDIR.drm}/amdgpu

SRCS= \
	amdgpu_bo.c \
	amdgpu_cs.c \
	amdgpu_device.c \
	amdgpu_gpu_info.c \
	amdgpu_vamgr.c \
	util_hash.c \
	util_hash_table.c

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11 -I${X11SRCDIR.drm} \
		-DHAVE_SYS_SYSCTL_H

LIBDPLIBS=	drm	${.CURDIR}/../libdrm

PKGCONFIG=	libdrm_amdgpu
PKGDIST=	drm

COPTS.amdgpu_cs.c+= -Wno-stack-protector

.include "../libdrm/Makefile.defines"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
