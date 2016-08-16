#	$NetBSD: Makefile,v 1.6 2016/08/16 06:24:01 mrg Exp $

DRIVER=		xf86-video-ark
DRIVER_NAME=	ark_drv

SRCS=		ark_accel.c ark_driver.c

CPPFLAGS+=	-DPACKAGE_VERSION=\"${PACKAGE_VERSION}\"

.include "../Makefile.xf86-driver"
