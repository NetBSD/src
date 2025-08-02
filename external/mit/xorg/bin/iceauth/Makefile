#	$NetBSD: Makefile,v 1.4.28.1 2025/08/02 05:50:38 perseant Exp $

.include <bsd.own.mk>

PROG=	iceauth
SRCS=	iceauth.c process.c

LDADD+=	-lICE
DPADD+=	${LIBICE}

CPPFLAGS+=	-DHAVE_REALLOCARRAY

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include "../../xorg-pkg-ver.mk"

.include <bsd.x11.mk>
.include <bsd.prog.mk>
