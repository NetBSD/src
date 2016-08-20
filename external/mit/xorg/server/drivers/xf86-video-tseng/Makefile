#	$NetBSD: Makefile,v 1.6 2016/08/20 05:44:01 mrg Exp $

DRIVER=		xf86-video-tseng
DRIVER_NAME=	tseng_drv

SRCS=		tseng_accel.c tseng_cursor.c tseng_dga.c
SRCS+=		tseng_driver.c tseng_mode.c
MAN=		tseng.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include

.if ${ACTIVE_CC} == "gcc"
COPTS+=		-Wno-error=cpp
.endif

CWARNFLAGS.clang+=	-Wno-pointer-sign

.include "../Makefile.xf86-driver"
