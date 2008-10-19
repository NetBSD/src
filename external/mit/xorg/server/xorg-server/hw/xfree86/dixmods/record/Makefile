#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:19 haad Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	record

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.record} recordmod.c

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/common

.include "../../../../record/Makefile.record"
