#	$NetBSD: Makefile,v 1.3 2004/02/07 06:52:07 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/input

.include "../../../../Makefile.serverlib"

MODULE=	mutouch_drv
MAN=	mutouch.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/mutouch

SRCS=	xf86MuTouch.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/mutouch \
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
