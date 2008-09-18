#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:45 wrstuden Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	fb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.fb} fbmodule.c fbcmap_mi.c

.include "../../../../fb/Makefile.fb"
