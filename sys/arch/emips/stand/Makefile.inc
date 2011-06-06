# $NetBSD: Makefile.inc,v 1.1.8.2 2011/06/06 09:05:20 jruoho Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
