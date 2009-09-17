#	$NetBSD: Makefile,v 1.7.4.2 2009/09/17 04:24:35 snj Exp $

DRIVER=		xf86-input-keyboard
DRIVER_NAME=	kbd_drv

SRCS=		kbd.c bsd_KbdMap.c bsd_kbd.c at_scancode.c
MAN=		kbd.4

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
CPPFLAGS+=	-DPCVT_SUPPORT
.endif
CPPFLAGS+=	-DWSCONS_SUPPORT
CPPFLAGS+=	-DXKB
CPPFLAGS.kbd.c=	-D__XKBDEFRULES__=${__XKBDEFRULES__}

X11EXTRAMANDEFS+=	-e 's,__xkb_path__,${X11LIBDIR}/xkb,g'

COPTS.kbd.c=      -Wno-error	# XXX deprecated

.include "../Makefile.xf86-driver"
