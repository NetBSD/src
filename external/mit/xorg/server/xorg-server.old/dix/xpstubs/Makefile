#	$NetBSD: Makefile,v 1.1.1.1 2016/06/10 03:42:14 mrg Exp $

.include "../../Makefile.serverlib"
.include "../../Makefile.servermod"

LIB=		xpstubs

.PATH:		${X11SRCDIR.xorg-server}/dix
SRCS=		xpstubs.c

CPPFLAGS+=	${X11FLAGS.DIX} ${X11INCS.DIX} \
		-I${DESTDIR}${X11INCDIR}/xorg

.include <bsd.x11.mk>
.include <bsd.lib.mk>
