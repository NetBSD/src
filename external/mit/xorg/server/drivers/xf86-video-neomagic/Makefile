#	$NetBSD: Makefile,v 1.4.6.1 2014/05/22 14:09:45 yamt Exp $

DRIVER=		xf86-video-neomagic
DRIVER_NAME=	neomagic_drv

SRCS=		neo_2070.c neo_2090.c neo_2097.c neo_2200.c
SRCS+=		neo_cursor.c neo_dga.c  neo_driver.c neo_i2c.c
SRCS+=		neo_shadow.c neo_video.c
MAN=		neomagic.4

COPTS.neo_driver.c=	-Wno-error	# XXX xf86dgastr.h

CPPFLAGS+=	-DHAVE_XAA_H

.include "../Makefile.xf86-driver"
