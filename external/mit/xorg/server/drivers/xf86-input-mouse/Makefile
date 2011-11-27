#	$NetBSD: Makefile,v 1.5 2011/11/27 00:50:37 tsutsui Exp $

WARNS?=		2

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c bsd_mouse.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"
