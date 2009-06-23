# $NetBSD: Makefile,v 1.3.6.1 2009/06/23 07:07:28 snj Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   i915drm

SRCS=	i915_dma.c
SRCS+=	i915_drv.c
SRCS+=	i915_irq.c
SRCS+=	i915_mem.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
