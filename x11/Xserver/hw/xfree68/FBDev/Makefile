#	$NetBSD: Makefile,v 1.1 2004/03/28 15:31:32 jdc Exp $

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	fbdev

SRCS=	fbdev_netbsd.c

CPPFLAGS+=	-I${X11SRCDIR.xc}/programs/Xserver/hw/netbsd/atari \
		-I${X11SRCDIR.xc}/programs/Xserver/mi \
		-I${X11SRCDIR.xc}/programs/Xserver/iplan2p4 \
		-I${X11SRCDIR.xc}/programs/Xserver/iplan2p8 \
		-I${X11SRCDIR.xc}/programs/Xserver/mfb \
		-I${X11SRCDIR.xc}/programs/Xserver/include \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86 \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/common \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support/bus \
		-I${DESTDIR}${X11INCDIR}/X11 \
		-I${DESTDIR}${X11INCDIR}/X11/extensions \
		-I${X11SRCDIR.xc}/include/fonts \
		-I${XSERVERINCDIR}

CPPFLAGS+=	-DCONFIG_IPLAN2p4 -DCONFIG_IPLAN2p8

.include <bsd.x11.mk>
.include <bsd.lib.mk>
