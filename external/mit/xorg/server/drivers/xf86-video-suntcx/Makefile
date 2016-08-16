#	$NetBSD: Makefile,v 1.3 2016/08/16 06:24:03 mrg Exp $

DRIVER=		xf86-video-suntcx
DRIVER_NAME=	suntcx_drv

SRCS=		tcx_driver.c tcx_cursor.c tcx_accel.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		suntcx.4

.include "../Makefile.xf86-driver"
