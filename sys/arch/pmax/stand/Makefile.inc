# $NetBSD: Makefile.inc,v 1.12.10.1 1999/12/27 18:33:30 wrstuden Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1
MKMAN=		no

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
