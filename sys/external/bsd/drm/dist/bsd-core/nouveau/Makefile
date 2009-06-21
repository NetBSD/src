# $FreeBSD$

.PATH:	${.CURDIR}/..
KMOD	= nouveau
NO_MAN	= YES
SRCS    = nouveau_drv.c nouveau_state.c nouveau_mem.c nouveau_object.c \
	  nouveau_sgdma.c nouveau_fifo.c nouveau_notifier.c nouveau_dma.c \
	  nouveau_irq.c nouveau_swmthd.c \
	  nv04_timer.c \
	  nv04_mc.c nv40_mc.c nv50_mc.c \
	  nv04_fb.c nv10_fb.c nv40_fb.c \
	  nv04_fifo.c nv10_fifo.c nv40_fifo.c nv50_fifo.c \
	  nv04_graph.c nv10_graph.c nv20_graph.c \
	  nv40_graph.c nv50_graph.c \
	  nv04_instmem.c nv50_instmem.c
#	  nouveau_bo.c nouveau_fence.c \
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
