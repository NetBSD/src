#	$NetBSD: Makefile,v 1.2 2003/12/24 12:44:17 rtr Exp $

XTODO= cpconfig

NOLINT=		1
NOPIC=		1
NOPROFILE=	1

.include <bsd.own.mk>

LIB=	xf86config
INCS=	xf86Parser.h xf86Optrec.h
INCSDIR=${X11INCDIR}

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/parser

SRCS=	DRI.c Device.c Files.c Flags.c Input.c Keyboard.c Layout.c Module.c \
	Video.c Monitor.c Pointer.c Screen.c Vendor.c read.c scan.c write.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${XSERVERINCDIR}

CPPFLAGS.scan.c=	-DXCONFIGDIR=\"/usr/X11R6/lib/X11\" \
			-DXCONFIGFILE=\"XF86Config\" \
			-DXVERSION="(((4) * 10000000) + (( 3) * 100000) + (( 0) * 1000) + 0)"

.include <bsd.x11.mk>
LIBDIR=${X11USRLIBDIR}
.include <bsd.lib.mk>
