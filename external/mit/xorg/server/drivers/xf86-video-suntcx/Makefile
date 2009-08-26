#	$NetBSD: Makefile,v 1.1 2009/08/26 22:30:46 macallan Exp $

DRIVER=		xf86-video-suntcx
DRIVER_NAME=	suntcx_drv

SRCS=		tcx_driver.c tcx_cursor.c tcx_accel.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		suntcx.4

.include "../Makefile.xf86-driver"
