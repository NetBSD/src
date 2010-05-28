#	$NetBSD: Makefile,v 1.1 2010/05/28 02:35:46 mrg Exp $

DRIVER=		xf86-video-openchrome
DRIVER_NAME=	openchrome_drv

SRCS=	\
	via_3d.c \
	via_accel.c \
	via_bandwidth.c \
	via_ch7xxx.c \
	via_cursor.c \
	via_crtc.c \
	via_dga.c \
	via_display.c \
	via_driver.c \
	via_i2c.c \
	via_id.c \
	via_lvds.c \
	via_memcpy.c \
	via_memory.c \
	via_mode.c \
	via_panel.c \
	via_shadow.c \
	via_swov.c \
	via_timing.c \
	via_vbe.c \
	via_vgahw.c \
	via_video.c \
	via_vt162x.c

# DRI
SRCS+=	\
	via_dri.c \
	via_xvmc.c

MAN=	openchrome.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/libdrm
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src

.include "../Makefile.xf86-driver"
