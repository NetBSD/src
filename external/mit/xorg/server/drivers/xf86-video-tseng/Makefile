#	$NetBSD: Makefile,v 1.5 2016/08/20 05:02:58 mrg Exp $

DRIVER=		xf86-video-tseng
DRIVER_NAME=	tseng_drv

SRCS=		tseng_accel.c tseng_cursor.c tseng_dga.c
SRCS+=		tseng_driver.c tseng_mode.c
MAN=		tseng.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include

COPTS+=		-Wno-error=cpp

CWARNFLAGS.clang+=	-Wno-pointer-sign

.include "../Makefile.xf86-driver"
