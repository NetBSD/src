#	$NetBSD: Makefile,v 1.4.4.2 2004/11/21 10:01:27 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	pcidata

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci
SRCS=	xf86PciData.c xf86PciIds.h
DPSRCS+=xf86PciIds.h

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bus \
                -I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/os \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xc}/programs/Xserver/Xext \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
                -I${X11SRCDIR.xc}/programs/Xserver/Xi \
		-I${X11SRCDIR.xc}/lib/font/include \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/parser \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader \
		-I${X11SRCDIR.xc}/programs/Xserver/rac \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DXORG_VERSION_CURRENT=${XORG_VERSION_CURRENT}

CPPFLAGS.xf86PciData.c+=	-DPCIDATA

BUILDSYMLINKS=	xf86ScanPci.c xf86PciData.c \
		xf86PciStdIds.h xf86PciIds.h

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
