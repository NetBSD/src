# $NetBSD: Makefile.inc,v 1.16 2002/05/07 02:04:26 simonb Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
