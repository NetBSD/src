#	$NetBSD: Makefile,v 1.3 2015/07/17 08:29:31 mrg Exp $

.include <bsd.own.mk>

PROG=	iceauth
SRCS=	iceauth.c process.c

CPPFLAGS+=	-I${X11SRCDIR.${PROG}}/../include -DHAVE_CONFIG_H

LDADD+=	-lICE
DPADD+=	${LIBICE}

.PATH:	${X11SRCDIR.${PROG}} ${X11SRCDIR.${PROG}}/man

.include <bsd.x11.mk>
.include <bsd.prog.mk>
