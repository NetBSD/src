#	$NetBSD: Makefile,v 1.4 2010/07/17 06:55:39 mrg Exp $

DRIVER=		xf86-video-neomagic
DRIVER_NAME=	neomagic_drv

SRCS=		neo_2070.c neo_2090.c neo_2097.c neo_2200.c
SRCS+=		neo_cursor.c neo_dga.c  neo_driver.c neo_i2c.c
SRCS+=		neo_shadow.c neo_video.c
MAN=		neomagic.4

COPTS.neo_driver.c=	-Wno-error	# XXX xf86dgastr.h

.include "../Makefile.xf86-driver"
