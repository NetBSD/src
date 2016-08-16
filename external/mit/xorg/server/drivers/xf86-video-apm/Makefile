#	$NetBSD: Makefile,v 1.3 2016/08/16 06:19:47 mrg Exp $

DRIVER=		xf86-video-apm
DRIVER_NAME=	apm_drv

SRCS=		apm_accel.c apm_cursor.c apm_dga.c apm_driver.c apm_i2c.c
MAN=		apm.4

CPPFLAGS+=	-DXV_NEW_REPUT

.include "../Makefile.xf86-driver"
