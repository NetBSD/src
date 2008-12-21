#	$NetBSD: Makefile,v 1.8 2008/12/21 21:20:03 jmcneill Exp $

DRIVER=		xf86-input-keyboard
DRIVER_NAME=	kbd_drv

SRCS=		kbd.c bsd_KbdMap.c bsd_kbd.c at_scancode.c
MAN=		kbd.4

.if ${MACHINE} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DPCVT_SUPPORT
.endif
CPPFLAGS+=	-DWSCONS_SUPPORT
CPPFLAGS+=	-DXKB
CPPFLAGS.kbd.c=	-D__XKBDEFRULES__=${__XKBDEFRULES__}

X11EXTRAMANDEFS+=	-e 's,__xkb_path__,${X11LIBDIR}/xkb,g'

.include "../Makefile.xf86-driver"
