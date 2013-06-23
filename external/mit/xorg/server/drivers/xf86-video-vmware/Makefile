#	$NetBSD: Makefile,v 1.6.12.1 2013/06/23 06:28:47 tls Exp $

DRIVER=		xf86-video-vmware
DRIVER_NAME=	vmware_drv

SRCS=	\
	bits2pixels.c \
	vmware.c \
	vmwarecurs.c \
	vmwarectrl.c \
	vmwarexinerama.c \
	vmwarevideo.c \
	vmwaremodes.c \
	vmware_bootstrap.c \
	vmware_common.c

MAN=	vmware.4

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/../include -DHAVE_CONFIG_H \
		-I${DESTDIR}${X11INCDIR}/libdrm

.include "../Makefile.xf86-driver"
