# $NetBSD: Makefile.inc,v 1.19.6.1 2005/01/17 19:29:11 skrll Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
