#	$NetBSD: Makefile,v 1.3.2.2 2008/09/18 05:18:42 wrstuden Exp $

DRIVER=		xf86-video-nsc
DRIVER_NAME=	nsc_drv

SRCS=		durango.c nsc_driver.c nsc_galstub.c nsc_gx1_accel.c
SRCS+=		nsc_gx1_cursor.c nsc_gx1_dga.c nsc_gx1_driver.c
SRCS+=		nsc_gx1_shadow.c nsc_gx1_video.c nsc_gx2_accel.c
SRCS+=		nsc_gx2_cursor.c nsc_gx2_dga.c nsc_gx2_driver.c
SRCS+=		nsc_gx2_shadow.c nsc_gx2_video.c panel.c
MAN=		nsc.4

CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src/gfx
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/src/panel

.include "../Makefile.xf86-driver"
