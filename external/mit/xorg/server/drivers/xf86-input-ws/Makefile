#	$NetBSD: Makefile,v 1.3.30.1 2019/01/18 08:49:04 pgoyette Exp $

WARNS?=		2

DRIVER=		xf86-input-ws
DRIVER_NAME=	ws_drv

SRCS=		ws.c emumb.c
MAN=		ws.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/../include \
		-I${X11SRCDIR.${DRIVER}}/include

CWARNFLAGS.clang+=	-Wno-parentheses-equality

COPTS.emumb.c=		-Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
