# $NetBSD: Makefile.inc,v 1.1 2011/01/26 01:18:53 pooka Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
