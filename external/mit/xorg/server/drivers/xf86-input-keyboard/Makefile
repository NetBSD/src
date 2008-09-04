#	$NetBSD: Makefile,v 1.6 2008/09/04 12:29:14 rtr Exp $

DRIVER=		xf86-input-keyboard
DRIVER_NAME=	kbd_drv

SRCS=		kbd.c bsd_KbdMap.c bsd_kbd.c at_scancode.c
MAN=		kbd.4

CPPFLAGS+=	-DWSCONS_SUPPORT
CPPFLAGS.kbd.c=	-D__XKBDEFRULES__=${__XKBDEFRULES__}

.include "../Makefile.xf86-driver"
