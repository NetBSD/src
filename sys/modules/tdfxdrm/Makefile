# $NetBSD: Makefile,v 1.1 2011/08/28 16:19:08 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   tdfxdrm
IOCONF=	tdfxdrm.ioconf

SRCS=	tdfx_drv.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
