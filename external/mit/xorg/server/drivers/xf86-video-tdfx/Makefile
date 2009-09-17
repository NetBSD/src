#	$NetBSD: Makefile,v 1.3.8.1 2009/09/17 04:24:36 snj Exp $

DRIVER=		xf86-video-tdfx
DRIVER_NAME=	tdfx_drv

SRCS=		tdfx_accel.c tdfx_dga.c tdfx_driver.c tdfx_hwcurs.c
SRCS+=		tdfx_io.c tdfx_priv.c tdfx_sli.c tdfx_video.c
SRCS+=		tdfx_dri.c
MAN=		tdfx.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

COPTS.tdfx_driver.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
