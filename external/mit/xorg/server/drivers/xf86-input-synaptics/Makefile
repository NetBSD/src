#	$NetBSD: Makefile,v 1.2 2018/12/31 12:13:50 mrg Exp $

DRIVER=		xf86-input-synaptics
DRIVER_NAME=	synaptics_drv

SRCS=		synaptics.c alpscomm.c ps2comm.c properties.c
#SRCS+=		psmcomm.c

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/include
CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

MAN=		synaptics.4

COPTS.synaptics.c+=		-Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
