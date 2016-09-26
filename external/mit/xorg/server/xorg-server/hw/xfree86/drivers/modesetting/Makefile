#	$NetBSD: Makefile,v 1.1 2016/09/26 22:37:32 mrg Exp $

DRIVER=		xf86-video-modesetting
DRIVER_NAME=	modesetting_drv

SRCS=		dri2.c \
		driver.c \
		drmmode_display.c \
		dumb_bo.c \
		present.c \
		sh3224.c \
		vblank.c 
MAN=		modesetting.4

CPPFLAGS+=	${X11FLAGS.DIX} \
		-I${DESTDIR}${X11INCDIR}/libdrm \
		-I${X11SRCDIR.xorg-server}/glamor \
		-I${X11SRCDIR.xorg-server}/ddc \
		-I${X11SRCDIR.xorg-server}/dri2 \
		-I${X11SRCDIR.xorg-server}/i2c \
		-I${X11SRCDIR.xorg-server}/modes \
		-I${X11SRCDIR.xorg-server}/parser \
		-I${X11SRCDIR.xorg-server}/ramdac \

# Set some overrides so Makefile.xf86-driver works for this
# built-in driver.

DRIVER_VERSION=		0
DRIVER_NO_FAKE_INCLUDE=	# defined

.include "../../../../../drivers/Makefile.xf86-driver"
