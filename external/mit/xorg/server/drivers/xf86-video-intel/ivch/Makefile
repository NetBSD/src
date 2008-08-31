#	$NetBSD: Makefile,v 1.1 2008/08/31 19:58:06 christos Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	ivch_drv


SRCS=		ivch_module.c ivch.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/ivch
