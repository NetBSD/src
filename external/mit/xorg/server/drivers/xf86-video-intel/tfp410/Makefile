#	$NetBSD: Makefile,v 1.1 2008/08/31 19:58:06 christos Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	tfp410_drv


SRCS=		tfp410_module.c tfp410.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/tfp410
