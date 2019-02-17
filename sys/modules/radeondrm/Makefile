# $NetBSD: Makefile,v 1.12 2019/02/17 04:05:57 rin Exp $

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

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

WARNS=	3

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.radeon_cs.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.kmodule.mk>
