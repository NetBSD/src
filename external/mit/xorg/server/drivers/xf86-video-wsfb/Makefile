#	$NetBSD: Makefile,v 1.2.8.1.4.1 2010/04/21 05:24:42 matt Exp $

DRIVER=		xf86-video-wsfb
DRIVER_NAME=	wsfb_drv

SRCS=		wsfb_driver.c wsfb_cursor.c
MAN=		wsfb.4

CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"
