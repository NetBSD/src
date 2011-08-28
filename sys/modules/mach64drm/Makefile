# $NetBSD: Makefile,v 1.1 2011/08/28 16:19:07 jmcneill Exp $

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

.include <bsd.kmodule.mk>
