#       $NetBSD: Makefile,v 1.1 2005/01/21 08:33:11 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../../Makefile.serverlib"

MODULE=	riva128
MAN=	#defined

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nv
SRCS=	riva_driver.c riva_dac.c riva_setup.c riva_cursor.c riva_xaa.c \
	riva_dga.c riva_shadow.c riva_hw.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/nv \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/fbdevhw \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf4bpp \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
	-I${X11SRCDIR.xc}/programs/Xserver/mfb \
	-I${X11SRCDIR.xc}/programs/Xserver/mi \
	-I${X11SRCDIR.xc}/programs/Xserver/fb \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xaa \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/rac \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10 \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vgahw \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
	-I${X11SRCDIR.xc}/programs/Xserver/Xext \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf24_32bpp \
	-I${X11SRCDIR.xc}/include/fonts \
	-I${X11SRCDIR.xc}/programs/Xserver/include \
	-I${DESTDIR}${X11INCDIR}/X11 \
	-I${DESTDIR}${X11INCDIR}/X11/extensions \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/shadowfb \
	-I${X11SRCDIR.xc}/programs/Xserver/render \
	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
	-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../../Makefile.drivermod"
