# $NetBSD: Makefile,v 1.3 2011/02/18 14:26:52 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   viadrm
IOCONF=	viadrm.ioconf

SRCS=	via_drv.c
SRCS+=	via_dma.c
SRCS+=	via_dmablit.c
SRCS+=	via_ds.c
SRCS+=	via_irq.c
SRCS+=	via_map.c
SRCS+=	via_mm.c
SRCS+=	via_verifier.c
SRCS+=	via_video.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
