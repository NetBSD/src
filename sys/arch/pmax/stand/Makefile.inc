# $NetBSD: Makefile.inc,v 1.13.8.2 2002/06/23 17:39:13 jdolecek Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
