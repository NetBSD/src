#	$NetBSD: Makefile,v 1.1.6.2 2008/10/19 22:41:15 haad Exp $

DRIVER=		xf86-video-intel
DRIVER_NAME=	ivch_drv


SRCS=		ivch_module.c ivch.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/drm

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/ivch
