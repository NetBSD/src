#	$NetBSD: Makefile,v 1.4.46.1 2019/06/10 22:02:45 christos Exp $

DRIVER=		xf86-video-wsfb
DRIVER_NAME=	wsfb_drv

SRCS=		wsfb_driver.c wsfb_cursor.c
MAN=		wsfb.4

CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

CWARNFLAGS.clang+=	-Wno-format

.include "../Makefile.xf86-driver"
