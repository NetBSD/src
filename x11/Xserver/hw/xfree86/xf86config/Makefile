#	$NetBSD: Makefile,v 1.1 2003/12/23 03:07:34 rtr Exp $

XTODO= cpconfig

.include "../../../Makefile.serverlib"

.include <bsd.own.mk>

LIB=	xf86config
.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/parser

SRCS=	DRI.c Device.c Files.c Flags.c Input.c Keyboard.c Layout.c Module.c \
	Video.c Monitor.c Pointer.c Screen.c Vendor.c read.c scan.c write.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${XSERVERINCDIR}

# XXX fixme please
CPPFLAGS.scan.c=	-DXCONFIGDIR=\"/usr/X11R6/lib/X11\" \
			-DXCONFIGFILE=\"XF86Config\" \
			-DXVERSION="(((4) * 10000000) + (( 3) * 100000) + (( 0) * 1000) + 0)"

.include <bsd.x11.mk>
.include <bsd.lib.mk>
