#	$NetBSD: Makefile,v 1.9 2018/12/31 12:13:50 mrg Exp $

WARNS?=		2

DRIVER=		xf86-input-mouse
DRIVER_NAME=	mouse_drv

SRCS=		mouse.c pnp.c bsd_mouse.c
MAN=		mousedrv.4

CPPFLAGS+=	-DHAVE_ASPRINTF
CPPFLAGS+=	-I${X11SRCDIR.${DRIVER}}/include

INCSDIR=	${X11INCDIR}/xorg
INCS=		xf86-mouse-properties.h

PKGCONFIG=	xorg-mouse
PKGDIST.xorg-mouse=	${X11SRCDIR.${DRIVER}}

.include "../Makefile.xf86-driver"

.PATH:  ${X11SRCDIR.${DRIVER}}/include
.PATH:  ${X11SRCDIR.${DRIVER}}

COPTS.mouse.c+=		-Wno-error	# XXX deprecated

XORG_PKG_VER_PROG=	${DRIVER}
.include "../../../xorg-pkg-ver.mk"
