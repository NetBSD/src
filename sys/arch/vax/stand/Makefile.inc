# $NetBSD: Makefile.inc,v 1.1.14.2 2002/08/27 23:46:02 nathanw Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
