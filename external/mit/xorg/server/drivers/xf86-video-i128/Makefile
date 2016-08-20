#	$NetBSD: Makefile,v 1.4 2016/08/20 05:44:01 mrg Exp $

DRIVER=		xf86-video-i128
DRIVER_NAME=	i128_drv

SRCS=		i128accel.c i128dga.c i128exa.c i128_driver.c
SRCS+=		i128IBMDAC.c i128init.c
MAN=		i128.4

.if ${ACTIVE_CC} == "gcc"
COPTS+=		-Wno-error=cpp
.endif

.include "../Makefile.xf86-driver"
