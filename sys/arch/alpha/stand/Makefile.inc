# $NetBSD: Makefile.inc,v 1.18.8.1 2002/08/30 00:18:52 gehenna Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
