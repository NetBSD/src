#	$NetBSD: Makefile,v 1.4 2016/08/12 01:15:50 mrg Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules
XMODULEDIRANDSTATIC=	# defined

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	fb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.fb} fbmodule.c

.include "../../../../fb/Makefile.fb"
