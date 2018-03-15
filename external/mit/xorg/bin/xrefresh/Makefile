#	$NetBSD: Makefile,v 1.2.26.1 2018/03/15 09:11:55 pgoyette Exp $

.include <bsd.own.mk>

PROG=	xrefresh

LDADD+=	-lXext -lX11
DPADD+=	${LIBXEXT} ${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
