#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:41 wrstuden Exp $

# XXX move this under ../xf86-video-cirrus

DRIVER=		xf86-video-cirrus
DRIVER_NAME=	cirrus_alpine_drv

SRCS=		alp_driver.c alp.h alp_hwcurs.c alp_i2c.c alp_xaa.c
SRCS+=		alp_xaam.c

#CPPFLAGS.cir_driver.c=	-DVERSION=${PACKAGE_MAJOR}

.include "../Makefile.xf86-driver"
