#	$NetBSD: Makefile,v 1.1 2005/03/23 11:09:58 martin Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	sunbw2_drv
MAN=	sunbw2.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/sunbw2

SRCS=	bw2_driver.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/sunbw2 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
                -I${DESTDIR}${X11INCDIR}/X11 \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
