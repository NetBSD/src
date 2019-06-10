# $NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:37 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core
.PATH:	${S}/external/bsd/drm/dist/shared-core

KMOD=   savagedrm
IOCONF=	savagedrm.ioconf

SRCS=	savage_drv.c
SRCS+=	savage_bci.c
SRCS+=	savage_state.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/bsd-core \
		-I${S}/external/bsd/drm/dist/shared-core

WARNS=	3

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.savage_state.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.kmodule.mk>
