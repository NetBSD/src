# $NetBSD: Makefile.inc,v 1.1.14.1 2002/01/08 00:28:32 nathanw Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
