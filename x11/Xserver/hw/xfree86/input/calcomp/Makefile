#	$NetBSD: Makefile,v 1.2 2004/02/07 06:52:06 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/input

.include "../../../../Makefile.serverlib"

MODULE=	calcomp_drv
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/calcomp

SRCS=	xf86Calcomp.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/calcomp \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
                -I${DESTDIR}${X11INCDIR}/X11 \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
                -I${XSERVERINCDIR}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
