# $NetBSD: Makefile.inc,v 1.13.12.1 2002/01/08 00:27:07 nathanw Exp $

NOMAN=		# defined

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
