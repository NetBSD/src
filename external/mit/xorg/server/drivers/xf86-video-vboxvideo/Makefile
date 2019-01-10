#	$NetBSD: Makefile,v 1.1 2019/01/10 00:05:57 mrg Exp $

DRIVER=		xf86-video-vboxvideo
DRIVER_NAME=	vboxvideo_drv

SRCS=	 \
		HGSMIBase.c \
		HGSMIBuffers.c \
		HGSMICommon.c \
		Modesetting.c \
		VBVABase.c \
		edid.c \
		getmode.c \
		helpers.c \
		hgsmimemalloc.c \
		pointer.c \
		setmode.c \
		vboxvideo.c \
		vbva.c

CPPFLAGS+=	\
		-I${X11SRCDIR.${DRIVER}}/src \
		-DXORG_7X \
		-DIN_RING3 \
		-DPCIACCESS \
		-DVBOXVIDEO_13 \
		-include ${DESTDIR}/usr/X11R7/include/xorg/xorg-server.h

MAN=		vboxvideo.4

.include "../Makefile.xf86-driver"
