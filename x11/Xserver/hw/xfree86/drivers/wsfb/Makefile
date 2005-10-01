#	$NetBSD: Makefile,v 1.3 2005/10/01 17:12:01 macallan Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	wsfb_drv
MAN=	wsfb.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/wsfb

SRCS=	wsfb_driver.c wsfb_cursor.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/dummy \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf1bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xf4bpp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/rac \
		-I${X11SRCDIR.xc}/programs/Xserver/miext/shadow \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
