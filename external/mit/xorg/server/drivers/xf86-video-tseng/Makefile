#	$NetBSD: Makefile,v 1.8 2016/09/02 04:49:48 jakllsch Exp $

DRIVER=		xf86-video-tseng
DRIVER_NAME=	tseng_drv

SRCS=		tseng_accel.c tseng_cursor.c tseng_dga.c
SRCS+=		tseng_driver.c tseng_mode.c
MAN=		tseng.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include

COPTS+=		${${ACTIVE_CC} == "gcc":? -Wno-error=cpp :}
COPTS+=		${${ACTIVE_CC} == "clang":? -Wno-error=\#warnings :}

CWARNFLAGS.clang+=	-Wno-pointer-sign

.include "../Makefile.xf86-driver"
