#	$NetBSD: Makefile,v 1.4 2008/08/03 14:47:26 rtr Exp $

DRIVER=		xf86-input-keyboard
DRIVER_NAME=	keyboard_drv

SRCS=		kbd.c bsd_KbdMap.c bsd_kbd.c at_scancode.c
MAN=		kbd.4

CPPFLAGS.kbd.c=	-D__XKBDEFRULES__=${__XKBDEFRULES__}

.include "../Makefile.xf86-driver"
