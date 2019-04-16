#	$NetBSD: Makefile,v 1.1 2019/04/16 21:21:51 mrg Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	glamoregl

.PATH:	${X11SRCDIR.xorg-server}/glamor
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/glamor_egl
SRCS=	glamor_egl.c \
	glamor_eglmodule.c \
	glamor_xf86_xv.c

CPPFLAGS+=	\
	-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
	-I${X11SRCDIR.xorg-server}/Xext \
	-I${X11SRCDIR.xorg-server}/mi \
	-I${X11SRCDIR.xorg-server}/include \
	-I${X11SRCDIR.xorg-server}/glamor \
	-I${X11SRCDIR.xorg-server}/../include \
	-I${DESTDIR}${X11INCDIR}/X11 \
	-I${DESTDIR}${X11INCDIR}/pixman-1 \
	-I${DESTDIR}${X11INCDIR}/xorg \
	-I${DESTDIR}${X11INCDIR}/libdrm \
	-I${X11SRCDIR.xorg-server}/hw/xfree86/ddc \
	-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c

CPPFLAGS+=	\
	-DGLAMOR_HAS_GBM

LIBDPLIBS=	\
	gbm	${.CURDIR}/../../../../../lib/libgbm

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
