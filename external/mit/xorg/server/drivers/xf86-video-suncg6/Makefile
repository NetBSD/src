#	$NetBSD: Makefile,v 1.6 2016/08/19 19:10:54 mrg Exp $

DRIVER=		xf86-video-suncg6
DRIVER_NAME=	suncg6_drv

SRCS=		cg6_driver.c cg6_cursor.c cg6_accel.c cg6_exa.c

CPPFLAGS+=	${X11FLAGS.DRI}

MAN=		suncg6.4

# XXX
COPTS.cg6_accel.c+=	-Wno-error=discarded-qualifiers

.include "../Makefile.xf86-driver"
