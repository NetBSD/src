#	$NetBSD: Makefile,v 1.3 2016/08/20 05:02:58 mrg Exp $

DRIVER=		xf86-video-i128
DRIVER_NAME=	i128_drv

SRCS=		i128accel.c i128dga.c i128exa.c i128_driver.c
SRCS+=		i128IBMDAC.c i128init.c
MAN=		i128.4

COPTS+=		-Wno-error=cpp

.include "../Makefile.xf86-driver"
