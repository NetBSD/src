# $NetBSD: Makefile.inc,v 1.1 2000/06/17 00:59:24 matt Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
