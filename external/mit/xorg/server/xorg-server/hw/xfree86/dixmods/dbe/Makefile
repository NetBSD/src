#	$NetBSD: Makefile,v 1.3.2.2 2008/09/18 05:18:45 wrstuden Exp $

XMODULEDIR=     ${X11USRLIBDIR}/modules/extensions

.include "../../../../Makefile.serverlib"
.include "../../../../Makefile.servermod"

LIB=	dbe

.PATH:	${X11SRCDIR.xorg-server}/hw/xfree86/dixmods
SRCS=	${SRCS.dbe} dbemodule.c

# XXX should we be installing xorg-config.h?
CPPFLAGS+=	-I${DESTDIR}${X11INCDIR}/xorg \
		-I${X11SRCDIR.xorg-server}/../include
CPPFLAGS+=	-DHAVE_XORG_CONFIG_H

.include "../../../../dbe/Makefile.dbe"
