#	$NetBSD: Makefile,v 1.5 2016/08/16 06:24:02 mrg Exp $

DRIVER=		xf86-video-suncg6
DRIVER_NAME=	suncg6_drv

SRCS=		cg6_driver.c cg6_cursor.c cg6_accel.c cg6_exa.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		suncg6.4

.include "../Makefile.xf86-driver"
