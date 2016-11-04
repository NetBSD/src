#	$NetBSD: Makefile,v 1.5.10.1 2016/11/04 14:48:48 pgoyette Exp $

DRIVER=		xf86-video-suncg14
DRIVER_NAME=	suncg14_drv

SRCS=		cg14_driver.c cg14_cursor.c cg14_accel.c cg14_render.c
MAN=		suncg14.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include
CWARNFLAGS.clang+=	-Wno-pointer-sign

.include "../Makefile.xf86-driver"
