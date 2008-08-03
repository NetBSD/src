#	$NetBSD: Makefile,v 1.1 2008/08/03 12:30:18 rtr Exp $

.include "../../../Makefile.serverlib"

LIB=		ddc

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=		xf86DDC.c edid.c interpret_edid.c print_edid.c ddcProperty.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c

.include <bsd.x11.mk>
#XXX and what shall we do about these
LIBDIR= ${XMODULEDIR}
.include <bsd.lib.mk>
