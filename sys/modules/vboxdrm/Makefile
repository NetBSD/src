#	$NetBSD: Makefile,v 1.1.62.1 2019/06/10 22:09:42 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/drm

KMOD=	vboxdrm
IOCONF=	vboxdrm.ioconf
SRCS=	vbox_drv.c

CPPFLAGS+= -I${S}/external/bsd/drm/dist/bsd-core
CPPFLAGS+= -I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
