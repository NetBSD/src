# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:54 rin Exp $

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

WARNS=	3

.include <bsd.kmodule.mk>
