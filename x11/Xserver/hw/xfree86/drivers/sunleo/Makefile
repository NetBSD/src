#	$NetBSD: Makefile,v 1.1 2005/03/23 11:09:59 martin Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/drivers

.include "../../../../Makefile.serverlib"

MODULE=	sunleo_drv
MAN=	sunleo.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/sunleo

SRCS=	leo_driver.c leo_accel.c leo_bcopy.c leo_checks.c leo_frect.c \
	leo_frectsp.c leo_fspans.c leo_fspanssp.c leo_gc.c leo_glyph.c \
	leo_stubs.c leo_cursor.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/drivers/sunleo \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/ramdac \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${X11SRCDIR.xc}/include/extensions \
                -I${DESTDIR}${X11INCDIR}/X11 \
		-I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
