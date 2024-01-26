#	$NetBSD: Makefile,v 1.2 2024/01/26 11:32:12 tsutsui Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	fb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.fb} fbmodule.c fbcmap_mi.c

.include "../../../../fb/Makefile.fb"
