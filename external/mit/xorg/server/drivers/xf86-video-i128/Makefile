#	$NetBSD: Makefile,v 1.6 2016/09/02 03:49:55 jakllsch Exp $

DRIVER=		xf86-video-i128
DRIVER_NAME=	i128_drv

SRCS=		i128accel.c i128dga.c i128exa.c i128_driver.c
SRCS+=		i128IBMDAC.c i128init.c
MAN=		i128.4

COPTS+=		${${ACTIVE_CC} == "gcc":? -Wno-error=cpp :}
COPTS+=		${${ACTIVE_CC} == "clang":? -Wno-error=\#warnings :}

.include "../Makefile.xf86-driver"
