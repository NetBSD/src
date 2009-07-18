# $FreeBSD$

.PATH:	${.CURDIR}/..
KMOD	= radeon
NO_MAN	= YES
SRCS    = r300_cmdbuf.c radeon_cp.c radeon_drv.c radeon_state.c radeon_irq.c \
	radeon_mem.c
SRCS   += device_if.h bus_if.h pci_if.h opt_drm.h
CFLAGS += ${DEBUG_FLAGS} -I. -I..

.if ${CC} != "icc"
CFLAGS += --param large-function-growth=1000
.endif

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
