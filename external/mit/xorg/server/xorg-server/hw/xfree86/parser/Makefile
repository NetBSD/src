#	$NetBSD: Makefile,v 1.1 2008/08/05 05:25:49 mrg Exp $

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	xf86parser	

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/parser
SRCS=	DRI.c Device.c Extensions.c Files.c Flags.c Input.c Layout.c \
	Module.c Monitor.c Pointer.c Screen.c Vendor.c Video.c read.c \
	scan.c write.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
                -I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${X11SRCDIR.xorg-server}/Xext \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
                -I${X11SRCDIR.xorg-server}/Xi \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/parser

CPPFLAGS+=	${X11FLAGS.DIX}

.include <bsd.x11.mk>
LIBDIR=	${XMODULEDIR}
.include <bsd.lib.mk>
