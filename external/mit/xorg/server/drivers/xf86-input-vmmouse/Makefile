#	$NetBSD: Makefile,v 1.1.14.1 2010/04/21 05:24:40 matt Exp $

DRIVER=		xf86-input-vmmouse
DRIVER_NAME=	vmmouse_drv

SRCS=		vmmouse.c vmmouse_client.c vmmouse_proto.c
MAN=		vmmouse.4
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/shared

COPTS.vmmouse.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
.PATH:  ${X11SRCDIR.${DRIVER}}/shared
