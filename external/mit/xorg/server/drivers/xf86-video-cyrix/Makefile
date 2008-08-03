#	$NetBSD: Makefile,v 1.3 2008/08/03 09:24:14 rtr Exp $

DRIVER=		xf86-video-cyrix
DRIVER_NAME=	cyrix_drv

SRCS=		cyrix_accel.c cyrix_bank.c cyrix_driver.c
SRCS+=		cyrix_helper.c cyrix_shadow.c
MAN=		cyrix.4

CPPFLAGS.cyrix_driver.c=	-DVERSION=${PACKAGE_MAJOR}

.include "../Makefile.xf86-driver"
