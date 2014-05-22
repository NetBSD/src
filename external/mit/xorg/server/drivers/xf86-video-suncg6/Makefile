#	$NetBSD: Makefile,v 1.2.8.1 2014/05/22 14:09:46 yamt Exp $

DRIVER=		xf86-video-suncg6
DRIVER_NAME=	suncg6_drv

SRCS=		cg6_driver.c cg6_cursor.c cg6_accel.c

CPPFLAGS+=	${X11FLAGS.DRI}
CPPFLAGS+=	-DHAVE_XAA_H

MAN=		suncg6.4

.include "../Makefile.xf86-driver"
