#	$NetBSD: Makefile,v 1.4 2011/07/23 23:27:44 mrg Exp $

DRIVER=		xf86-input-vmmouse
DRIVER_NAME=	vmmouse_drv

SRCS=		vmmouse.c vmmouse_client.c vmmouse_proto.c
MAN=		vmmouse.4
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/shared
CPPFLAGS+=	-DXINPUT -DHAVE_XORG_SERVER_1_5_0

COPTS.vmmouse.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
.PATH:  ${X11SRCDIR.${DRIVER}}/shared
