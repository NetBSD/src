# $NetBSD: Makefile.inc,v 1.19 2002/08/27 08:53:14 lukem Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
