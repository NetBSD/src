# $NetBSD: Makefile,v 1.3.16.1 2010/04/21 00:28:20 matt Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   i915drm

SRCS=	i915_drv.c
SRCS+=	i915_dma.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c
SRCS+=	i915_suspend.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
