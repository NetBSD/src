#	$NetBSD: Makefile,v 1.4 2022/07/12 02:42:40 mrg Exp $

DRIVER=		xf86-video-cirrus
DRIVER_NAME=	cirrus_drv

SRCS=		cir_dga.c cir_driver.c CirrusClk.c cir_shadow.c
SRCS+=		alp_driver.c alp.h alp_hwcurs.c alp_i2c.c alp_xaa.c
SRCS+=		alp_xaam.c
SRCS+=		lg_driver.c lg_hwcurs.c lg_i2c.c lg_xaa.c

MAN=		cirrus.4

CPPFLAGS.cir_driver.c=	-DVERSION=${PACKAGE_MAJOR}

.include "../Makefile.xf86-driver"
