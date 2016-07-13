#	$NetBSD: Makefile,v 1.4.24.1 2016/07/13 19:34:23 snj Exp $

DRIVER=		xf86-video-ati
DRIVER_NAME=	ati_drv

SRCS=		ati.c atimodule.c
MAN=		ati.4

LIBDPLIBS+=	 drm_radeon ${.CURDIR}/../../../lib/libdrm_radeon

.include "../Makefile.xf86-driver"
