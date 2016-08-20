#	$NetBSD: Makefile,v 1.7 2016/08/20 06:49:21 mrg Exp $

DRIVER=		xf86-video-i740
DRIVER_NAME=	i740_drv

SRCS=		i740_accel.c i740_cursor.c i740_dga.c i740_driver.c
SRCS+=		i740_i2c.c i740_io.c i740_video.c
MAN=		i740.4

COPTS+=		${${ACTIVE_CC} == "gcc":? -Wno-error=cpp :}

.include "../Makefile.xf86-driver"
