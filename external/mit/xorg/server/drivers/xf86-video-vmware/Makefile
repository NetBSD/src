#	$NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:17 haad Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmware_drv

SRCS=		bits2pixels.c vmware.c vmwarecurs.c vmwarectrl.c
SRCS+=		vmwarexinerama.c vmwarevideo.c
MAN=		vmware.4

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

.include "../Makefile.xf86-driver"
