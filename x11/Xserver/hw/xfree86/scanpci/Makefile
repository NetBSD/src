#	$NetBSD: Makefile,v 1.3 2004/02/08 23:27:38 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"

LIB=	scanpci	

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/scanpci
SRCS=	xf86ScanPci.c xf86PciIds.h
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
		-I. \
		-I${XSERVERINCDIR}

BUILDSYMLINKS=	xf86PciStdIds.h xf86PciIds.h

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
