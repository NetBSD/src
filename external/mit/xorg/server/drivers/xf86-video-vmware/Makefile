#	$NetBSD: Makefile,v 1.3 2008/08/03 09:24:14 rtr Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmware_drv

SRCS=		bits2pixels.c vmware.c vmwarecurs.c vmwarectrl.c
SRCS+=		vmwarexinerama.c vmwarevideo.c
MAN=		vmware.4

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

.include "../Makefile.xf86-driver"
