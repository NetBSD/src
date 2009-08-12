# $NetBSD: Makefile,v 1.3 2008/07/08 10:11:02 jmmv Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/drm

KMOD=   i915drm

SRCS=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c

CPPFLAGS+=	-I${S}/dev/drm

.include <bsd.kmodule.mk>
