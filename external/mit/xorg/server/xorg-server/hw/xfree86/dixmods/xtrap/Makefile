#	$NetBSD: Makefile,v 1.3 2008/08/26 04:05:35 rtr Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=		XTrap

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=		${SRCS.XTrap} xf86XTrapModule.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg 

.include "../../../../XTrap/Makefile.xtrap"
