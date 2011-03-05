# $NetBSD: Makefile.inc,v 1.1.4.2 2011/03/05 20:49:59 rmind Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC=	-static
