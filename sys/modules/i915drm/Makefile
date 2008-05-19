# $NetBSD: Makefile,v 1.2 2008/05/19 13:21:57 tnn Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/drm

KMOD=   i915drm

SRCS=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c

.include <bsd.kmodule.mk>
