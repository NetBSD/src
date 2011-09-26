#	$NetBSD: Makefile,v 1.2 2011/09/26 18:08:09 tsutsui Exp $

WARNS?=		2

DRIVER=		xf86-input-ws
DRIVER_NAME=	ws_drv

SRCS=		ws.c emumb.c
MAN=		ws.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include \
		-I${X11SRCDIR.${DRIVER}}/include

.include "../Makefile.xf86-driver"
