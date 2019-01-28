#	$NetBSD: Makefile,v 1.2 2019/01/28 18:50:39 mrg Exp $

# This is for xorg-server 1.10 only.

DRIVER=		xf86-video-modesetting
DRIVER_NAME=	modesetting_drv

SRCS=		driver.c drmmode_display.c
MAN=		modesetting.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-DRANDR_12_INTERFACE

.include "../Makefile.xf86-driver"

.if ${HAVE_XORG_SERVER_VER} != "110"
.error "This subdirectory is for xorg-server 1.10 only."
.endif
