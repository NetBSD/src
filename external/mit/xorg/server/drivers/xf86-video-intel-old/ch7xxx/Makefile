#	$NetBSD: Makefile,v 1.1 2015/05/21 00:12:10 rjs Exp $

DRIVER=		xf86-video-intel-old
DRIVER_NAME=	ch7xxx_drv


SRCS=		ch7xxx_module.c ch7xxx.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11
CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/X11/dri

.include "../../Makefile.xf86-driver"
.PATH: ${X11SRCDIR.${DRIVER}}/src/ch7xxx
