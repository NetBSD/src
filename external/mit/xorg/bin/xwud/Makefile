#	$NetBSD: Makefile,v 1.2.14.1 2018/03/15 09:11:56 pgoyette Exp $

.include <bsd.own.mk>

PROG=	xwud

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
