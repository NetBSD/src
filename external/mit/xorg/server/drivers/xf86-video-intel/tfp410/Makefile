#	$NetBSD: Makefile,v 1.2 2009/10/08 19:06:51 snj Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	tfp410_drv


SRCS=		tfp410_module.c tfp410.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/tfp410
