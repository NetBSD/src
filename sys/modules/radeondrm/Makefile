# $NetBSD: Makefile,v 1.16 2023/06/03 21:30:23 lukem Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   radeondrm
IOCONF=	radeondrm.ioconf

SRCS=	radeon_drv.c
SRCS+=	r300_cmdbuf.c
SRCS+=	r600_cp.c
SRCS+=	r600_blit.c
SRCS+=	radeon_cp.c
SRCS+=	radeon_cs.c
SRCS+=	radeon_irq.c
SRCS+=	radeon_mem.c
SRCS+=	radeon_state.c

# Code imported from Linux - expects signed overflow to be OK.
COPTS+=	-fwrapv

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

WARNS=	3

COPTS.radeon_cs.c+=	${CC_WNO_IMPLICIT_FALLTHROUGH}

CWARNFLAGS.clang+=	-Wno-error=bool-operation

.include <bsd.kmodule.mk>
