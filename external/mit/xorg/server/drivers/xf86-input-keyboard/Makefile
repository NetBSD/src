#	$NetBSD: Makefile,v 1.1 2008/08/02 06:32:02 mrg Exp $

DRIVER=		xf86-input-keyboard
DRIVER_NAME=	keyboard_drv

SRCS=		kbd.c bsd_KbdMap.c bsd_kbd.c at_scancode.c
MAN=		keyboard.4

# XXX kbd.c doesn't yet build

.include "../Makefile.xf86-driver"
