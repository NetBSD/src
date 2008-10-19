#	$NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:16 haad Exp $

DRIVER=		xf86-video-tdfx
DRIVER_NAME=	tdfx_drv

SRCS=		tdfx_accel.c tdfx_dga.c tdfx_driver.c tdfx_hwcurs.c
SRCS+=		tdfx_io.c tdfx_priv.c tdfx_sli.c tdfx_video.c
MAN=		tdfx.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

.include "../Makefile.xf86-driver"
