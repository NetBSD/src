#	$NetBSD: Makefile,v 1.2 2003/12/26 04:15:16 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/input

.include "../../../../Makefile.serverlib"

.include <bsd.own.mk>

MODULE=	mouse_drv
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/mouse
SRCS=	mouse.c pnp.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/mouse \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/loader \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
                -I${DESTDIR}${X11INCDIR}/X11 \
                -I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bus

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include "../../Makefile.drivermod"
