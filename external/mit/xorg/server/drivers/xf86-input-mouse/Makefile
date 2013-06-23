#	$NetBSD: Makefile,v 1.5.6.1 2013/06/23 06:28:46 tls Exp $

WARNS?=		2

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c bsd_mouse.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_CONFIG_H \
		-I${X11SRCDIR.${DRIVER}}/include \
		-I${X11SRCDIR.${DRIVER}}/../include

INCSDIR=	${X11INCDIR}/xorg
INCS=		xf86-mouse-properties.h

PKGCONFIG=	xorg-mouse
PKGDIST.xorg-mouse=	${X11SRCDIR.${DRIVER}}

.include "../Makefile.xf86-driver"

.PATH:  ${X11SRCDIR.${DRIVER}}/include
.PATH:  ${X11SRCDIR.${DRIVER}}
