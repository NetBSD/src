# $NetBSD: Makefile,v 1.2 2008/05/19 13:21:57 tnn Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/pci/drm

KMOD=   radeondrm

SRCS=	ati_pcigart.c
SRCS+=	r300_cmdbuf.c
SRCS+=	radeon_cp.c
SRCS+=	radeon_drv.c
SRCS+=	radeon_irq.c
SRCS+=	radeon_mem.c
SRCS+=	radeon_state.c

.include <bsd.kmodule.mk>
