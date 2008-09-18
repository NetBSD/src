#	$NetBSD: Makefile,v 1.3.2.2 2008/09/18 05:18:41 wrstuden Exp $

DRIVER=		xf86-video-glint
DRIVER_NAME=	glint_drv

SRCS=		glint_dga.c glint_driver.c glint_shadow.c IBMramdac.c
SRCS+=		pm2_accel.c pm2_dac.c pm2ramdac.c pm2v_dac.c pm2_video.c
SRCS+=		pm2vramdac.c pm3_accel.c pm3_dac.c pm3_video.c pm_accel.c
SRCS+=		pm_dac.c sx_accel.c TIramdac.c tx_accel.c tx_dac.c
MAN=		glint.4

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/X11/drm

.include "../Makefile.xf86-driver"
