# $NetBSD: Makefile.inc,v 1.23 2011/01/23 13:24:05 joerg Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
