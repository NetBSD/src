#	$NetBSD: Makefile,v 1.1 2008/09/25 14:15:06 rtr Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	glx

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.glx} glxmodule.c

.include "../../../../GL/glx/Makefile.glx"
