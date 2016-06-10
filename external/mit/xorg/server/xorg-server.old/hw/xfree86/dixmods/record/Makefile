#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	record

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.record} recordmod.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/common

.include "../../../../record/Makefile.record"
