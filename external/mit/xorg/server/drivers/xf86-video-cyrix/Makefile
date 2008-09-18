#	$NetBSD: Makefile,v 1.3.2.2 2008/09/18 05:18:41 wrstuden Exp $

DRIVER=		xf86-video-cyrix
DRIVER_NAME=	cyrix_drv

SRCS=		cyrix_accel.c cyrix_bank.c cyrix_driver.c
SRCS+=		cyrix_helper.c cyrix_shadow.c
MAN=		cyrix.4

CPPFLAGS.cyrix_driver.c=	-DVERSION=${PACKAGE_MAJOR}

.include "../Makefile.xf86-driver"
