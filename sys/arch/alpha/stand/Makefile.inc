# $NetBSD: Makefile.inc,v 1.17 2001/12/12 01:49:33 tv Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

NOMAN=			# defined

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
