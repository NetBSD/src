#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:41:15 haad Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	ch7017_drv


SRCS=		ch7017_module.c ch7017.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/ch7017
