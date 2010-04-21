#	$NetBSD: Makefile,v 1.3.14.1 2010/04/21 05:24:42 matt Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmware_drv

SRCS=		bits2pixels.c vmware.c vmwarecurs.c vmwarectrl.c
SRCS+=		vmwaremodes.c vmwarevideo.c vmwarexinerama.c 
MAN=		vmware.4

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include -DHAVE_CONFIG_H

.include "../Makefile.xf86-driver"
