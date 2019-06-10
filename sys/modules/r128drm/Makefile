# $NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:37 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   r128drm
IOCONF=	r128drm.ioconf

SRCS=	r128_drv.c
SRCS+=	r128_cce.c
SRCS+=	r128_irq.c
SRCS+=	r128_state.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

WARNS=	3

.include <bsd.kmodule.mk>
