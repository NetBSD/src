# $NetBSD: Makefile.inc,v 1.12.4.1 2000/11/20 20:20:40 bouyer Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1
MKMAN=		no

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
