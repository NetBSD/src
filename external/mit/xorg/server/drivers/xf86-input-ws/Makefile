#	$NetBSD: Makefile,v 1.1 2009/11/28 12:10:24 mbalmer Exp $

DRIVER=		xf86-input-ws
DRIVER_NAME=	ws_drv

SRCS=		ws.c emumb.c
MAN=		ws.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include \
		-I${X11SRCDIR.${DRIVER}}/include

COPTS.ws.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
