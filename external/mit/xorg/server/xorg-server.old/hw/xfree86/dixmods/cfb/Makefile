#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions
PIXELSIZE=	8

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	cfb

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.cfb} cfbmodule.c

.include "../../../../cfb/Makefile.cfb"
