# $NetBSD: Makefile,v 1.3.14.2 2009/05/04 08:14:10 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/drm

KMOD=   i915drm

SRCS=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c

CPPFLAGS+=	-I${S}/dev/drm

.include <bsd.kmodule.mk>
