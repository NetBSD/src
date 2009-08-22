#	$NetBSD: Makefile,v 1.4 2009/08/22 02:40:27 mrg Exp $

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

COPTS.dbe.c+=		-Wno-error

.include "../../../../dbe/Makefile.dbe"
