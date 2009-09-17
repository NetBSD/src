#	$NetBSD: Makefile,v 1.3.8.1 2009/09/17 04:24:37 snj Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmware_drv

SRCS=		bits2pixels.c vmware.c vmwarecurs.c vmwarectrl.c
SRCS+=		vmwaremodes.c vmwarevideo.c vmwarexinerama.c 
MAN=		vmware.4

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include -DHAVE_CONFIG_H

.include "../Makefile.xf86-driver"
