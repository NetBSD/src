#	$NetBSD: Makefile,v 1.8 2018/12/31 13:05:07 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=		ddc

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=		ddc.c interpret_edid.c print_edid.c ddcProperty.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support/bus \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/miext/damage \
		-I${X11SRCDIR.xorg-server}/render \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/i2c \
		-I${X11SRCDIR.xorg-server}/../include

CWARNFLAGS.clang+=	-Wno-format

.include <bsd.x11.mk>
.include <bsd.lib.mk>
