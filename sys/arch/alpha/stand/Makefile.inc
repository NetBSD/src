# $NetBSD: Makefile.inc,v 1.16.16.1 2002/01/10 19:37:08 thorpej Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
