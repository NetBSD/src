#	$NetBSD: Makefile,v 1.4 2013/06/19 13:27:07 macallan Exp $

DRIVER=		xf86-video-suncg14
DRIVER_NAME=	suncg14_drv

SRCS=		cg14_driver.c cg14_cursor.c cg14_accel.c
MAN=		suncg14.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include
CPPFLAGS+=	-DHAVE_XAA_H

.include "../Makefile.xf86-driver"
