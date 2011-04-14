#	$NetBSD: Makefile,v 1.2 2011/04/14 16:50:05 macallan Exp $

DRIVER=		xf86-video-suncg14
DRIVER_NAME=	suncg14_drv

SRCS=		cg14_driver.c cg14_cursor.c
MAN=		suncg14.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"
