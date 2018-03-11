#	$NetBSD: Makefile,v 1.3 2018/03/11 08:46:31 mrg Exp $

.include <bsd.own.mk>

PROG=	xrefresh

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
