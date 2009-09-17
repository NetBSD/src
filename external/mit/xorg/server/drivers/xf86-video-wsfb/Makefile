#	$NetBSD: Makefile,v 1.2.8.2 2009/09/17 04:24:37 snj Exp $

DRIVER=		xf86-video-wsfb
DRIVER_NAME=	wsfb_drv

SRCS=		wsfb_driver.c wsfb_cursor.c
MAN=		wsfb.4

CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"
