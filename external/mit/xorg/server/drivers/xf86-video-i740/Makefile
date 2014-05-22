#	$NetBSD: Makefile,v 1.2.4.1 2014/05/22 14:09:45 yamt Exp $

DRIVER=		xf86-video-i740
DRIVER_NAME=	i740_drv

SRCS=		i740_accel.c i740_cursor.c i740_dga.c i740_driver.c
SRCS+=		i740_i2c.c i740_io.c i740_video.c
MAN=		i740.4

CPPFLAGS+=	-DHAVE_XAA_H

.include "../Makefile.xf86-driver"
