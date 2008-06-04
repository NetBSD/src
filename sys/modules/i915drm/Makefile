# $NetBSD: Makefile,v 1.2.4.2 2008/06/04 02:05:47 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/drm

KMOD=   i915drm

SRCS=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c

.include <bsd.kmodule.mk>
