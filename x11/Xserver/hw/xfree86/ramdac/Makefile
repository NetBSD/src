# $NetBSD: Makefile,v 1.3 2004/02/07 06:52:07 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

LIB=	ramdac
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB}

SRCS=	BT.c IBM.c TI.c xf86BitOrder.c xf86Cursor.c xf86HWCurs.c \
	xf86RamDac.c xf86RamDacCmap.c xf86RamDacMod.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${X11SRCDIR.xc}/programs/Xserver/cfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

CPPFLAGS.xf86BitOrder.c=	-DXAAReverseBitOrder=xf86ReverseBitOrder -DRAMDAC_MODULE

BUILDSYMLINKS=	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/xaa/xaaBitOrder.c xf86BitOrder.c

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
