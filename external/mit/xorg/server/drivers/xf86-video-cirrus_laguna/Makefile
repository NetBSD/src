#	$NetBSD: Makefile,v 1.1 2008/08/03 12:56:44 rtr Exp $

DRIVER=		xf86-video-cirrus
DRIVER_NAME=	cirrus_laguna_drv

SRCS=		lg_driver.c lg_hwcurs.c lg_i2c.c lg_xaa.c

CPPFLAGS.lg_driver.c=	-Wno-error # XXX

.include "../Makefile.xf86-driver"
