# $NetBSD: Makefile.inc,v 1.16.16.2 2002/09/06 08:31:35 jdolecek Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
