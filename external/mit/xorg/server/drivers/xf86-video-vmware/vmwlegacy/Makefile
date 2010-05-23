#	$NetBSD: Makefile,v 1.1 2010/05/23 06:37:32 mrg Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmwlegacy_drv

SRCS=		bits2pixels.c vmware.c vmwarecurs.c vmwarectrl.c
SRCS+=		vmwaremodes.c vmwarevideo.c vmwarexinerama.c 

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include -DHAVE_CONFIG_H \
		-I${DESTDIR}${X11INCDIR}/libdrm

.include "../../Makefile.xf86-driver"
