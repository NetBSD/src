# $NetBSD: Makefile.inc,v 1.15.4.1 1999/06/21 00:46:10 thorpej Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
