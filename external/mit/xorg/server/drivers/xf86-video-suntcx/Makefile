#	$NetBSD: Makefile,v 1.4 2016/09/27 19:15:09 joerg Exp $

DRIVER=		xf86-video-suntcx
DRIVER_NAME=	suntcx_drv

SRCS=		tcx_driver.c tcx_cursor.c tcx_accel.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		suntcx.4

CWARNFLAGS.clang+=	-Wno-pointer-sign

.include "../Makefile.xf86-driver"
