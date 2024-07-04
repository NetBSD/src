#	$NetBSD: Makefile,v 1.5 2024/07/04 01:02:24 mrg Exp $

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
