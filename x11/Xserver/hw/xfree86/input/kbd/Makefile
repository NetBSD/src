#	$NetBSD: Makefile,v 1.4 2004/02/07 06:52:07 lukem Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/input

.include "../../../../Makefile.serverlib"

MODULE=	kbd_drv
MAN=	kbd.4 keyboard.4
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/keyboard

SRCS=	kbd.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/input/keyboard \
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
