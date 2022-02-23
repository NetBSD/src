#	$NetBSD: Makefile,v 1.7 2022/02/23 17:28:31 mrg Exp $

DRIVER=		xf86-video-ati-kms
DRIVER_NAME=	ati_drv

SRCS=		ati.c atimodule.c
MAN=		ati.4

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${X11SRCDIR.${DRIVER}}/../include

LIBDPLIBS+=	 drm_radeon ${.CURDIR}/../../../lib/libdrm_radeon

.include "../Makefile.xf86-driver"

# XXX
.if ${XORG_SERVER_SUBDIR} == "xorg-server.old"
CPPFLAGS+=	-DUSE_XAA
.endif
