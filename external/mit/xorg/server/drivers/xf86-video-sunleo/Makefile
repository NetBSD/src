#	$NetBSD: Makefile,v 1.3 2016/08/16 06:24:03 mrg Exp $

DRIVER=		xf86-video-sunleo
DRIVER_NAME=	sunleo_drv

SRCS=		leo_accel.c leo_cursor.c leo_driver.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		sunleo.4

.include "../Makefile.xf86-driver"
