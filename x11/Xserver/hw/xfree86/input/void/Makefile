#	$NetBSD: Makefile,v 1.1 2003/12/27 03:14:56 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/input

.include "../../../../Makefile.serverlib"

.include <bsd.own.mk>

MODULE=	void_drv
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/void

SRCS=	void.c

XSERVERINCDIR!= cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/void \
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
