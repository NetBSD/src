#	$NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:41:19 haad Exp $

XMODULEDIR=	${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	glx

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.glx} glxmodule.c

.include "../../../../GL/glx/Makefile.glx"
