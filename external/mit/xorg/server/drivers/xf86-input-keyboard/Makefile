#	$NetBSD: Makefile,v 1.3 2008/08/03 06:18:24 rtr Exp $

DRIVER=		xf86-input-keyboard
DRIVER_NAME=	keyboard_drv

SRCS=		kbd.c bsd_KbdMap.c bsd_kbd.c at_scancode.c
MAN=		kbd.4

CPPFLAGS.kbd.c=	-D__XKBDEFRULES__=${__XKBDEFRULES__}
CPPFLAGS.kbd.c+=-DPACKAGE_VERSION_MAJOR=${PACKAGE_MAJOR}
CPPFLAGS.kbd.c+=-DPACKAGE_VERSION_MINOR=${PACKAGE_MINOR}
CPPFLAGS.kbd.c+=-DPACKAGE_VERSION_PATCHLEVEL=${PACKAGE_PATCH}

.include "../Makefile.xf86-driver"
