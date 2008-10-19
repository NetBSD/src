#	$NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:15 haad Exp $

DRIVER=		xf86-video-cyrix
DRIVER_NAME=	cyrix_drv

SRCS=		cyrix_accel.c cyrix_bank.c cyrix_driver.c
SRCS+=		cyrix_helper.c cyrix_shadow.c
MAN=		cyrix.4

CPPFLAGS.cyrix_driver.c=	-DVERSION=${PACKAGE_MAJOR}

.include "../Makefile.xf86-driver"
