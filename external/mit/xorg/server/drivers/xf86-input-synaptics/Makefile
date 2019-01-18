#	$NetBSD: Makefile,v 1.1.44.1 2019/01/18 08:49:04 pgoyette Exp $

DRIVER=		xf86-input-synaptics
DRIVER_NAME=	synaptics_drv

SRCS=		synaptics.c alpscomm.c ps2comm.c properties.c
#SRCS+=		psmcomm.c

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/include
CPPFLAGS+=	-DHAVE_CONFIG_H -I${X11SRCDIR.${DRIVER}}/../include

MAN=		synaptics.4

COPTS.synaptics.c+=		-Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
