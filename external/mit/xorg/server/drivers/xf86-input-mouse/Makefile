#	$NetBSD: Makefile,v 1.4 2009/06/12 04:00:56 mrg Exp $

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c bsd_mouse.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include

COPTS.mouse.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
