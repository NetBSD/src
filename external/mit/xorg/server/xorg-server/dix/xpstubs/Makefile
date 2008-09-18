#	$NetBSD: Makefile,v 1.2.2.2 2008/09/18 05:18:44 wrstuden Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		xpstubs

.PATH:		${X11SRCDIR.xorg-server}/dix
SRCS=		xpstubs.c

CPPFLAGS+=	${X11FLAGS.DIX} ${X11INCS.DIX} \
		-I${DESTDIR}${X11INCDIR}/xorg

.include <bsd.x11.mk>
.include <bsd.lib.mk>
