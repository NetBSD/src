#	$NetBSD: Makefile,v 1.3 2024/01/27 10:57:05 tsutsui Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules
XMODULEDIRANDSTATIC=	# defined

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	fb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.fb} fbmodule.c fbcmap_mi.c

.include "../../../../fb/Makefile.fb"
