#	$NetBSD: Makefile,v 1.6 2018/12/31 12:13:50 mrg Exp $

DRIVER=		xf86-input-vmmouse
DRIVER_NAME=	vmmouse_drv

SRCS=		vmmouse.c vmmouse_client.c vmmouse_proto.c
MAN=		vmmouse.4
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/shared
CPPFLAGS+=	-DXINPUT -DHAVE_XORG_SERVER_1_5_0
CPPFLAGS+=	-DABS_VALUATOR_AXES -DNO_MOUSE_MODULE

COPTS.vmmouse.c+=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
.PATH:  ${X11SRCDIR.${DRIVER}}/shared
