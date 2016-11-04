#	$NetBSD: Makefile,v 1.2.10.1 2016/11/04 14:48:49 pgoyette Exp $

DRIVER=		xf86-video-suntcx
DRIVER_NAME=	suntcx_drv

SRCS=		tcx_driver.c tcx_cursor.c tcx_accel.c

CPPFLAGS+=	${X11FLAGS.DRI}
CPPFLAGS+=	-DHAVE_XAA_H

MAN=		suntcx.4

CWARNFLAGS.clang+=	-Wno-pointer-sign

.include "../Makefile.xf86-driver"
