# $NetBSD: Makefile.inc,v 1.16.2.1 2002/08/31 13:45:40 gehenna Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
