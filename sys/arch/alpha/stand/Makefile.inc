# $NetBSD: Makefile.inc,v 1.21 2005/01/05 12:18:18 simonb Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
