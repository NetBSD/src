#	$NetBSD: Makefile,v 1.1 2008/08/02 04:32:00 mrg Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		xpstubs

.PATH:		${X11SRCDIR.xorg-server}/dix
SRCS=		xpstubs.c

CPPFLAGS+=	${X11FLAGS.DIX} ${X11INCS.DIX}

.include <bsd.x11.mk>
.include <bsd.lib.mk>
