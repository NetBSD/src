#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:45 wrstuden Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	record

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.record} recordmod.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/common

.include "../../../../record/Makefile.record"
