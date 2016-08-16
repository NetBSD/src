#	$NetBSD: Makefile,v 1.6 2016/08/16 06:24:02 mrg Exp $

DRIVER=		xf86-video-suncg14
DRIVER_NAME=	suncg14_drv

SRCS=		cg14_driver.c cg14_cursor.c cg14_accel.c cg14_render.c
MAN=		suncg14.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"
