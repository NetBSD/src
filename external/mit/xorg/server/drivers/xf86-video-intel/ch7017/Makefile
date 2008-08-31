#	$NetBSD: Makefile,v 1.1 2008/08/31 19:58:06 christos Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	ch7017_drv


SRCS=		ch7017_module.c ch7017.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/ch7017
