#	$NetBSD: Makefile,v 1.4.2.1 2004/11/15 09:12:43 rtr Exp $

XTODO= cpconfig

NOLINT=		1
NOPIC=		1
NOPROFILE=	1

.include <bsd.own.mk>

LIB=	xf86config
INCS=	xf86Parser.h xf86Optrec.h
INCSDIR=${X11INCDIR}

.PATH:	${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/parser

SRCS=	Device.c Files.c Flags.c Input.c Keyboard.c Layout.c Module.c \
        Video.c Monitor.c Pointer.c Screen.c Vendor.c read.c scan.c \
	write.c DRI.c Extensions.c

XSERVERINCDIR!=	cd ${NETBSDSRCDIR}/x11/Xserver/include && ${PRINTOBJDIR}

CPPFLAGS=	-I${X11SRCDIR.xc}/programs/Xserver/hw/xfree86/os-support \
		-I${XSERVERINCDIR}

CPPFLAGS.scan.c=	-DXCONFIGDIR=\"${X11ETCDIR}\" \
			-DXCONFIGFILE=\"XF86Config\" \
			-DXVERSION="${XVERSION}"

.include <bsd.x11.mk>
XVERSION!=	${PRINTX11VERSION}
LIBDIR=${X11USRLIBDIR}
.include <bsd.lib.mk>
