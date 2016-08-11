#	$NetBSD: Makefile,v 1.3 2016/08/11 08:32:43 mrg Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	record

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.record}

CPPFLAGS+=	-I${X11SRCDIR.xorg-server}/hw/xfree86/common

.include "../../../../record/Makefile.record"
