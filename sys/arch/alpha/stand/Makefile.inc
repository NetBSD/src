# $NetBSD: Makefile.inc,v 1.16.14.2 2002/08/27 23:44:23 nathanw Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
