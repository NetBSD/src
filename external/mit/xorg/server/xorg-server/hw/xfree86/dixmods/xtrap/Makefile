#	$NetBSD: Makefile,v 1.2 2008/08/24 13:25:03 rtr Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	XTrap

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.XTrap} xf86XTrapModule.c

.include "../../../../XTrap/Makefile.xtrap"
