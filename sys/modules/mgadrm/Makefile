# $NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:32 christos Exp $

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
