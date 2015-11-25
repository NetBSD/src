#	$NetBSD: Makefile,v 1.4 2015/11/25 18:02:08 macallan Exp $

DRIVER=		xf86-video-suncg6
DRIVER_NAME=	suncg6_drv

SRCS=		cg6_driver.c cg6_cursor.c cg6_accel.c cg6_exa.c

CPPFLAGS+=	${X11FLAGS.DRI}
CPPFLAGS+=	-DHAVE_XAA_H

MAN=		suncg6.4

.include "../Makefile.xf86-driver"
