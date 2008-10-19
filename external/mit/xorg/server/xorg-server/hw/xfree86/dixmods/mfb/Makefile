#	$NetBSD: Makefile,v 1.3.6.2 2008/10/19 22:41:19 haad Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=		mfb

.PATH:		${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=		${SRCS.mfb} mfbmodule.c

CPPFLAGS+=      -I${DESTDIR}${X11INCDIR}/xorg 

.include "../../../../mfb/Makefile.mfb"
