#	$NetBSD: Makefile,v 1.1.10.1 2014/05/22 14:09:46 yamt Exp $

DRIVER=		xf86-video-sunleo
DRIVER_NAME=	sunleo_drv

SRCS=		leo_accel.c leo_cursor.c leo_driver.c

CPPFLAGS+=	${X11FLAGS.DRI}
CPPFLAGS+=	-DHAVE_XAA_H

MAN=		sunleo.4

.include "../Makefile.xf86-driver"
