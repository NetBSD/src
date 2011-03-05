#	$NetBSD: Makefile,v 1.1.4.2 2011/03/05 20:55:48 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/drm

KMOD=	vboxdrm
IOCONF=	vboxdrm.ioconf
SRCS=	vbox_drv.c

CPPFLAGS+= -I${S}/external/bsd/drm/dist/bsd-core
CPPFLAGS+= -I${S}/external/bsd/drm/dist/shared-core

WARNS=	4

.include <bsd.kmodule.mk>
