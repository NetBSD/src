#	$NetBSD: Makefile,v 1.3 2013/06/04 22:45:07 mrg Exp $

DRIVER=		xf86-video-suncg14
DRIVER_NAME=	suncg14_drv

SRCS=		cg14_driver.c cg14_cursor.c
MAN=		suncg14.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include
CPPFLAGS+=	-DHAVE_XAA_H

.include "../Makefile.xf86-driver"
