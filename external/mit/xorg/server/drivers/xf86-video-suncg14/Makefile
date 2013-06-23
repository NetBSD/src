#	$NetBSD: Makefile,v 1.2.10.1 2013/06/23 06:28:47 tls Exp $

DRIVER=		xf86-video-suncg14
DRIVER_NAME=	suncg14_drv

SRCS=		cg14_driver.c cg14_cursor.c cg14_accel.c
MAN=		suncg14.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include
CPPFLAGS+=	-DHAVE_XAA_H

.include "../Makefile.xf86-driver"
