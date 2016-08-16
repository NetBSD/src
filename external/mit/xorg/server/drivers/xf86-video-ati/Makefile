#	$NetBSD: Makefile,v 1.6 2016/08/16 06:24:01 mrg Exp $

DRIVER=		xf86-video-ati
DRIVER_NAME=	ati_drv

SRCS=		ati.c atimodule.c
MAN=		ati.4

LIBDPLIBS+=	 drm_radeon ${.CURDIR}/../../../lib/libdrm_radeon

.include "../Makefile.xf86-driver"

# XXX
.if ${XORG_SERVER_SUBDIR} == "xorg-server.old"
CPPFLAGS+=	-DUSE_XAA
.endif
