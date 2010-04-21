# $FreeBSD$

.PATH:	${.CURDIR}/..
KMOD	= mach64
NO_MAN	= YES
SRCS    = mach64_dma.c mach64_drv.c mach64_irq.c mach64_state.c
SRCS   += device_if.h bus_if.h pci_if.h opt_drm.h
CFLAGS += ${DEBUG_FLAGS} -I. -I..

.if defined(DRM_DEBUG)
DRM_DEBUG_OPT= "\#define DRM_DEBUG 1"
.endif

.if !defined(DRM_NOLINUX)
DRM_LINUX_OPT= "\#define DRM_LINUX 1"
.endif

opt_drm.h:
	touch opt_drm.h
	echo $(DRM_DEBUG_OPT) >> opt_drm.h
	echo $(DRM_LINUX_OPT) >> opt_drm.h

.include <bsd.kmod.mk>
