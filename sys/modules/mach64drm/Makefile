# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:54 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   mach64drm
IOCONF=	mach64drm.ioconf

SRCS=	mach64_drv.c
SRCS+=	mach64_dma.c
SRCS+=	mach64_irq.c
SRCS+=	mach64_state.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

WARNS=	3

.include <bsd.kmodule.mk>
