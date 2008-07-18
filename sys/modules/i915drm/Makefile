# $NetBSD: Makefile,v 1.2.6.1 2008/07/18 16:37:57 simonb Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/drm

KMOD=   i915drm

SRCS=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c

CPPFLAGS+=	-I${S}/dev/drm

.include <bsd.kmodule.mk>
