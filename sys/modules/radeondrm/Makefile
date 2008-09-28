# $NetBSD: Makefile,v 1.2.2.3 2008/09/28 10:40:55 mjf Exp $

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
