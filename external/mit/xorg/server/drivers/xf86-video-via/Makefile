#	$NetBSD: Makefile,v 1.3.8.1 2009/09/17 04:24:37 snj Exp $

DRIVER=		xf86-video-via
DRIVER_NAME=	via_drv

SRCS=		via_3d.c via_accel.c via_bandwidth.c via_cursor.c
SRCS+=		via_dga.c via_driver.c via_i2c.c via_id.c via_memcpy.c
SRCS+=		via_memory.c via_mode.c via_shadow.c via_swov.c
SRCS+=		via_vbe.c via_vgahw.c via_video.c via_vt162x.c
SRCS+=		via_dri.c via_xvmc.c
MAN=		via.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src

.include "../Makefile.xf86-driver"
