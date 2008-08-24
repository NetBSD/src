#	$NetBSD: Makefile,v 1.2 2008/08/24 13:30:20 rtr Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	mfb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.mfb} mfbmodule.c

.include "../../../../mfb/Makefile.mfb"
