# $NetBSD: Makefile.inc,v 1.13.12.2 2002/06/20 03:40:30 nathanw Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
