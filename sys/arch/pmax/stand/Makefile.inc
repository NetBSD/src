# $NetBSD: Makefile.inc,v 1.17 2002/08/27 08:53:17 lukem Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
