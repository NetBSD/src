# $NetBSD: Makefile.inc,v 1.22.108.1 2011/02/08 16:19:10 bouyer Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
