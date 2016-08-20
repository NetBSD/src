#	$NetBSD: Makefile,v 1.5 2016/08/20 05:02:58 mrg Exp $

DRIVER=		xf86-video-i740
DRIVER_NAME=	i740_drv

SRCS=		i740_accel.c i740_cursor.c i740_dga.c i740_driver.c
SRCS+=		i740_i2c.c i740_io.c i740_video.c
MAN=		i740.4

COPTS+=		-Wno-error=cpp

.include "../Makefile.xf86-driver"
