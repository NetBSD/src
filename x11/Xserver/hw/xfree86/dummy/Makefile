#	$NetBSD: Makefile,v 1.4 2004/07/23 16:28:18 sketch Exp $

.include "../../../Makefile.serverlib"


LIB=	dummy

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/dummylib
SRCS=	fatalerror.c getvalidbios.c logvwrite.c pcitestmulti.c \
	verrorf.c xalloc.c xf86allocscripi.c xf86addrestolist.c \
	xf86drvmsg.c xf86drvmsgverb.c xf86errorf.c xf86errorfverb.c \
	xf86getpagesize.c xf86getverb.c xf86info.c xf86msg.c \
	xf86msgverb.c xf86opt.c xf86screens.c xf86servisinit.c \
	xf86verbose.c sigiostubs.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/dummylib \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${XSERVERINCDIR}

BUILDSYMLINKS=	../os-support/shared/sigiostubs.c sigiostubs.c

.include <bsd.x11.mk>
.include <bsd.lib.mk>
