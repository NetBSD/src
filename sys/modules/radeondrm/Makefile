# $NetBSD: Makefile,v 1.5.8.1 2009/07/23 23:32:47 jym Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   radeondrm

SRCS=	radeon_drv.c
# XXX this one should be in a sub-driver to share with r128drm
SRCS+=	ati_pcigart.c
SRCS+=	r300_cmdbuf.c
SRCS+=	r600_cp.c
SRCS+=	radeon_cp.c
SRCS+=	radeon_irq.c
SRCS+=	radeon_mem.c
SRCS+=	radeon_state.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
