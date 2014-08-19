#	$NetBSD: Makefile,v 1.4.28.1 2014/08/20 00:00:14 tls Exp $

DRIVER=		xf86-video-ati
DRIVER_NAME=	ati_drv

SRCS=		ati.c atimodule.c
MAN=		ati.4

LIBDPLIBS+=	 drm_radeon ${.CURDIR}/../../../lib/libdrm_radeon

.include "../Makefile.xf86-driver"
