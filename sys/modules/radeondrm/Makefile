# $NetBSD: Makefile,v 1.6 2009/06/22 13:13:08 tron Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   radeondrm

SRCS=	ati_pcigart.c
SRCS+=	r300_cmdbuf.c
SRCS+=	radeon_cp.c
SRCS+=	radeon_drv.c
SRCS+=	radeon_irq.c
SRCS+=	radeon_mem.c
SRCS+=	radeon_state.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
