#	$NetBSD: Makefile,v 1.3 2018/03/11 08:59:52 mrg Exp $

.include <bsd.own.mk>

PROG=	xwud

LDADD+=	-lX11
DPADD+=	${LIBX11}

.PATH:	${X11SRCDIR.${PROG}}
.PATH:	${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
