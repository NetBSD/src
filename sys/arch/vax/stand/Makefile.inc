# $NetBSD: Makefile.inc,v 1.3 2001/12/12 12:24:26 lukem Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
