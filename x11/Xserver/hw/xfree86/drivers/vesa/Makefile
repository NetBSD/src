#	$NetBSD: Makefile,v 1.3 2004/02/07 06:52:06 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	vesa_drv
MAN=	vesa.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/vesa

SRCS=	vesa.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/vesa \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf4bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf24_32bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/afb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/shadow \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vgahw \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/rac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ddc \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/i2c \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/vbe \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/int10 \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
