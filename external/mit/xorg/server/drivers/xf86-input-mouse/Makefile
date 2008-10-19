#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:14 haad Exp $

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include

.include "../Makefile.xf86-driver"
