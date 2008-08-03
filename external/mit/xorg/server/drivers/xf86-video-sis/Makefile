#	$NetBSD: Makefile,v 1.3 2008/08/03 09:24:14 rtr Exp $

DRIVER=		xf86-video-sis
DRIVER_NAME=	sis_drv

SRCS=		init301.c init.c initextx.c  sis300_accel.c
SRCS+=		sis310_accel.c sis6326_video.c sis_accel.c sis_cursor.c
SRCS+=		sis_dac.c sis_dga.c sis_driver.c sis_memcpy.c sis_opt.c
SRCS+=		sis_setup.c sis_shadow.c sis_utility.c sis_vb.c
SRCS+=		sis_vga.c sis_video.c
MAN=		sis.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

.include "../Makefile.xf86-driver"
