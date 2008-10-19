#	$NetBSD: Makefile,v 1.7.2.2 2008/10/19 22:41:14 haad Exp $

DRIVER=		xf86-input-keyboard
DRIVER_NAME=	kbd_drv

SRCS=		kbd.c bsd_KbdMap.c bsd_kbd.c at_scancode.c
MAN=		kbd.4

CPPFLAGS+=	-DWSCONS_SUPPORT
CPPFLAGS.kbd.c=	-D__XKBDEFRULES__=${__XKBDEFRULES__}

X11EXTRAMANDEFS+=	-e 's,__xkb_path__,${X11LIBDIR}/xkb,g'

.include "../Makefile.xf86-driver"
