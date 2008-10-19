#	$NetBSD: Makefile,v 1.4.6.2 2008/10/19 22:41:19 haad Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=		xtrap

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=		${SRCS.XTrap} xf86XTrapModule.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg 

.include "../../../../XTrap/Makefile.xtrap"
