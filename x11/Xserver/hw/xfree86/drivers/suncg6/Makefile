#	$NetBSD: Makefile,v 1.1 2005/03/23 11:09:59 martin Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	suncg6_drv
MAN=	suncg6.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/suncg6

SRCS=	cg6_driver.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/suncg6 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/fb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/render \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/include/extensions \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
