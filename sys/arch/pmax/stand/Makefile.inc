# $NetBSD: Makefile.inc,v 1.13.12.3 2002/08/27 23:45:02 nathanw Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
