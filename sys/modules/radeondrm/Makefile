# $NetBSD: Makefile,v 1.1 2008/05/18 21:13:22 jmcneill Exp $

.PATH:	../../dev/pci/drm

KMOD=   radeondrm

SRCS=	ati_pcigart.c
SRCS+=	r300_cmdbuf.c
SRCS+=	radeon_cp.c
SRCS+=	radeon_drv.c
SRCS+=	radeon_irq.c
SRCS+=	radeon_mem.c
SRCS+=	radeon_state.c

.include <bsd.kmodule.mk>
