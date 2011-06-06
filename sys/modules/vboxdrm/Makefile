#	$NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:09:51 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/dev/drm

KMOD=	vboxdrm
IOCONF=	vboxdrm.ioconf
SRCS=	vbox_drv.c

CPPFLAGS+= -I${S}/external/bsd/drm/dist/bsd-core
CPPFLAGS+= -I${S}/external/bsd/drm/dist/shared-core

WARNS=	4

.include <bsd.kmodule.mk>
