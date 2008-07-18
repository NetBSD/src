# $NetBSD: Makefile,v 1.2.6.1 2008/07/18 16:37:57 simonb Exp $

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

CPPFLAGS+=	-I${S}/dev/drm

.include <bsd.kmodule.mk>
