#	$NetBSD: Makefile,v 1.1 2008/08/03 23:00:41 mrg Exp $

MKX11LOADABLE=no

.include "../../../Makefile.serverlib"
.include "../../../Makefile.servermod"

LIB=	dummy

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dummylib
SRCS=	fatalerror.c getvalidbios.c getemptypci.c logvwrite.c \
	pcitestmulti.c verrorf.c xalloc.c \
	xf86allocscripi.c xf86addrestolist.c xf86drvmsg.c \
	xf86drvmsgverb.c xf86errorf.c xf86errorfverb.c \
	xf86getpagesize.c xf86getverb.c xf86info.c xf86msg.c \
	xf86msgverb.c xf86opt.c xf86screens.c xf86servisinit.c \
	xf86verbose.c

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/os-support/shared/
SRCS+=	sigiostubs.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/dummylib \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/common \
		-I${X11SRCDIR.xorg-server}/hw/xfree86/os-support \
		-I${X11SRCDIR.xorg-server}/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/pixman-1 \
		-I${DESTDIR}${X11INCDIR}/xorg \
		-I${XSERVERINCDIR}

COPTS.fatalerror.c+=	-Wno-error
COPTS.xf86info.c+=	-Wno-error

.include <bsd.x11.mk>
.include <bsd.lib.mk>
