#	$NetBSD: Makefile,v 1.2.2.1 2012/04/17 00:04:50 yamt Exp $

WARNS?=		2

DRIVER=		xf86-input-ws
DRIVER_NAME=	ws_drv

SRCS=		ws.c emumb.c
MAN=		ws.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include \
		-I${X11SRCDIR.${DRIVER}}/include

CWARNFLAGS.clang+=	-Wno-parentheses-equality

.include "../Makefile.xf86-driver"
