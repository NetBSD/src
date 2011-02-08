# $NetBSD: Makefile.inc,v 1.1.2.2 2011/02/08 18:05:08 bouyer Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
