#	$NetBSD: Makefile,v 1.3.8.1 2009/09/17 04:24:41 snj Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	vgahw

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=	vgaHW.c vgaHWmodule.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/vgahw \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/cfb \
		-I${X11SRCDIR.xorg-server}/mfb \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ddc \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
