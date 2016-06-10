#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions
PIXELSIZE=	32

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	cfb32

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.cfb} cfb32module.c

.include "../../../../cfb/Makefile.cfb"
