# $NetBSD: Makefile.inc,v 1.13.8.3 2002/09/06 08:38:53 jdolecek Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
