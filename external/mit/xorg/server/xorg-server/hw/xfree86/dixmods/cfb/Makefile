#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:45 wrstuden Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions
PIXELSIZE=	8

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	cfb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.cfb} cfbmodule.c

.include "../../../../cfb/Makefile.cfb"
