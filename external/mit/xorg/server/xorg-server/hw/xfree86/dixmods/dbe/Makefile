#	$NetBSD: Makefile,v 1.6 2016/08/11 08:32:43 mrg Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	dbe

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.dbe}

CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/xorg
CPPFLAGS+=	-DHAVE_XORG_CONFIG_H ${X11INCS.DIX}

COPTS.dbe.c+=		-Wno-error

.include "../../../../dbe/Makefile.dbe"
