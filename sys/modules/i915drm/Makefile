# $NetBSD: Makefile,v 1.2.8.1 2008/10/19 22:17:41 haad Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/drm

KMOD=   i915drm

SRCS=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c

CPPFLAGS+=	-I${S}/dev/drm

.include <bsd.kmodule.mk>
