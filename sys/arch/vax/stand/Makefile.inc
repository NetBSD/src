# $NetBSD: Makefile.inc,v 1.1.10.1 2002/01/10 19:50:04 thorpej Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
