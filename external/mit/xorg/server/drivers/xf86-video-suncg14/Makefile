#	$NetBSD: Makefile,v 1.2.10.2 2014/08/20 00:00:14 tls Exp $

DRIVER=		xf86-video-suncg14
DRIVER_NAME=	suncg14_drv

SRCS=		cg14_driver.c cg14_cursor.c cg14_accel.c cg14_render.c
MAN=		suncg14.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include
CPPFLAGS+=	-DHAVE_XAA_H

.include "../Makefile.xf86-driver"
