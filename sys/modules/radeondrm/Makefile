# $NetBSD: Makefile,v 1.4 2008/10/15 06:51:20 wrstuden Exp $

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
