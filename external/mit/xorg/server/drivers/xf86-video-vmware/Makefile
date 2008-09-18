#	$NetBSD: Makefile,v 1.3.2.2 2008/09/18 05:18:43 wrstuden Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmware_drv

SRCS=		bits2pixels.c vmware.c vmwarecurs.c vmwarectrl.c
SRCS+=		vmwarexinerama.c vmwarevideo.c
MAN=		vmware.4

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

.include "../Makefile.xf86-driver"
