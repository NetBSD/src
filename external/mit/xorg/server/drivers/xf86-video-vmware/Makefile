#	$NetBSD: Makefile,v 1.6 2010/05/23 06:37:32 mrg Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmware_drv

SRCS=		vmwaremodule.c
MAN=		vmware.4

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include -DHAVE_CONFIG_H \
		-I${DESTDIR}${X11INCDIR}/libdrm

.include "../Makefile.xf86-driver"

SUBDIR= vmwlegacy

.include <bsd.subdir.mk>
