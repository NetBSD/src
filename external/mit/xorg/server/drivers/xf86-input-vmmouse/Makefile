#	$NetBSD: Makefile,v 1.2 2009/06/10 08:56:33 mrg Exp $

DRIVER=		xf86-input-vmmouse
DRIVER_NAME=	vmmouse_drv

SRCS=		vmmouse.c vmmouse_client.c vmmouse_proto.c
MAN=		vmmouse.4
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/shared

.include "../Makefile.xf86-driver"
.PATH:  ${X11SRCDIR.${DRIVER}}/shared
