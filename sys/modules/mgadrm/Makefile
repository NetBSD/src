# $NetBSD: Makefile,v 1.1 2011/08/28 16:19:07 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   mgadrm
IOCONF=	mgadrm.ioconf

SRCS=	mga_drv.c
SRCS+=	mga_dma.c
SRCS+=	mga_irq.c
SRCS+=	mga_state.c
SRCS+=	mga_warp.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
