#	$NetBSD: Makefile,v 1.2.14.1 2010/04/21 05:24:39 matt Exp $

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c bsd_mouse.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include

COPTS.mouse.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
