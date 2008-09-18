#	$NetBSD: Makefile,v 1.1.2.2 2008/09/18 05:18:42 wrstuden Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	ch7017_drv


SRCS=		ch7017_module.c ch7017.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/ch7017
