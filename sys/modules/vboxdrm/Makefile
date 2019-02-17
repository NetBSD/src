#	$NetBSD: Makefile,v 1.2 2019/02/17 04:06:00 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/drm

KMOD=	vboxdrm
IOCONF=	vboxdrm.ioconf
SRCS=	vbox_drv.c

CPPFLAGS+= -I${S}/external/bsd/drm/dist/bsd-core
CPPFLAGS+= -I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
