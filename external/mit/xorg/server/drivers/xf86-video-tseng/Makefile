#	$NetBSD: Makefile,v 1.3 2009/06/10 02:39:29 mrg Exp $

DRIVER=		xf86-video-tseng
DRIVER_NAME=	tseng_drv

SRCS=		tseng_accel.c tseng_cursor.c tseng_dga.c
SRCS+=		tseng_driver.c tseng_mode.c
MAN=		tseng.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"
