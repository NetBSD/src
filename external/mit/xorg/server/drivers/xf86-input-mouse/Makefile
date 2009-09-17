#	$NetBSD: Makefile,v 1.2.8.1 2009/09/17 04:24:35 snj Exp $

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c bsd_mouse.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include

COPTS.mouse.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
