#	$NetBSD: Makefile,v 1.2 2004/02/07 06:52:06 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/input

.include "../../../../Makefile.serverlib"

MODULE=	digitaledge_drv
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/digitaledge

SRCS=	DigitalEdge.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/digitaledge \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
                -I${DESTDIR}${X11INCDIR}/X11 \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
                -I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
