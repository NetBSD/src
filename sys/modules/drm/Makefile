# $NetBSD: Makefile,v 1.2.4.2 2008/06/04 02:05:47 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/drm

KMOD=   drm

SRCS=	drm_agpsupport.c
SRCS+=	drm_auth.c
SRCS+=	drm_bufs.c
SRCS+=	drm_context.c
SRCS+=	drm_dma.c
SRCS+=	drm_drawable.c
SRCS+=	drm_drv.c
SRCS+=	drm_fops.c
SRCS+=	drm_ioctl.c
SRCS+=	drm_irq.c
SRCS+=	drm_lock.c
SRCS+=	drm_memory.c
SRCS+=	drm_pci.c
SRCS+=	drm_scatter.c
SRCS+=	drm_sysctl.c
SRCS+=	drm_vm.c

.include <bsd.kmodule.mk>
