#	$NetBSD: Makefile,v 1.12.14.1 2019/06/10 22:02:45 christos Exp $

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

# XXX
COPTS.vmware.c+=	-Wno-error	# XXX deprecated
COPTS.vmwaremodes.c+=	-Wno-error

.include "../Makefile.xf86-driver"
