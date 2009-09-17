#	$NetBSD: Makefile,v 1.2.8.1 2009/09/17 04:24:38 snj Exp $

LIBISPRIVATE=yes

.include "../../../Makefile.serverlib"

LIB=		ddc

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=		xf86DDC.c edid.c interpret_edid.c print_edid.c ddcProperty.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
