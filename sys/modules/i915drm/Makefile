# $NetBSD: Makefile,v 1.3.10.1 2009/07/23 23:32:46 jym Exp $

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
