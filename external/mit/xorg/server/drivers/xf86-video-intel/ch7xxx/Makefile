#	$NetBSD: Makefile,v 1.1.8.1 2009/10/10 19:53:23 sborrill Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	ch7xxx_drv


SRCS=		ch7xxx_module.c ch7xxx.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/ch7xxx
