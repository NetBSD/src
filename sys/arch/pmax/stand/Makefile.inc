# $NetBSD: Makefile.inc,v 1.13.8.1 2002/01/10 19:47:56 thorpej Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
