#	$NetBSD: Makefile,v 1.2.6.2 2008/10/19 22:41:18 haad Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		xpstubs

.PATH:		${X11SRCDIR.xorg-server}/dix
SRCS=		xpstubs.c

CPPFLAGS+=	${X11FLAGS.DIX} ${X11INCS.DIX} \
		-I${DESTDIR}${X11INCDIR}/xorg

.include <bsd.x11.mk>
.include <bsd.lib.mk>
