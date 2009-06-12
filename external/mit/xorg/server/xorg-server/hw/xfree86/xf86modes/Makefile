# $NetBSD: Makefile,v 1.3 2009/06/12 01:59:06 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	xf86modes

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/modes
SRCS=	xf86Crtc.c xf86Cursors.c xf86cvt.c xf86gtf.c xf86DiDGA.c \
	xf86EdidModes.c xf86Modes.c xf86RandR12.c xf86Rotate.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ddc \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/loader \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/rac \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/parser \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/scanpci \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/vbe \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/int10 \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/vgahw \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/ramdac \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/dixmods/extmod \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg

CPPFLAGS+=	${X11FLAGS.DIX}

# XXX
CPPFLAGS.xf86RandR12.c=	-Wno-error

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
