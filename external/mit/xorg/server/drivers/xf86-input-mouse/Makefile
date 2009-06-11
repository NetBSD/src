#	$NetBSD: Makefile,v 1.3 2009/06/11 15:54:18 mrg Exp $

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include

COPTS.mouse.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
