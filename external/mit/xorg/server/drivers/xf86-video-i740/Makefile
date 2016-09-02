#	$NetBSD: Makefile,v 1.8 2016/09/02 03:49:55 jakllsch Exp $

DRIVER=		xf86-video-i740
DRIVER_NAME=	i740_drv

SRCS=		i740_accel.c i740_cursor.c i740_dga.c i740_driver.c
SRCS+=		i740_i2c.c i740_io.c i740_video.c
MAN=		i740.4

COPTS+=		${${ACTIVE_CC} == "gcc":? -Wno-error=cpp :}
COPTS+=		${${ACTIVE_CC} == "clang":? -Wno-error=\#warnings :}

.include "../Makefile.xf86-driver"
