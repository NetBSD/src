# $NetBSD: Makefile.inc,v 1.13 1999/11/28 00:39:30 simonb Exp $

.include <bsd.own.mk>

BINDIR?=	/usr/mdec

WARNS?=		1
MKMAN=		no

CPPFLAGS+=	-I${.CURDIR}/../..
LDSTATIC?=	-static
