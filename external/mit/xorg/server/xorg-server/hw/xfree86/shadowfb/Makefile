# $NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:20 haad Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	shadowfb
.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/${LIB}
SRCS=	sfbmodule.c shadow.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/${LIB} \
		-I${X11SRCDIR.xorg-server}/mi \
		-I${X11SRCDIR.xorg-server}/include \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/render \
		-I${DESTDIR}${X11INCDIR}/X11/extensions

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
