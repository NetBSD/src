# $NetBSD: Makefile.inc,v 1.1.10.2 2002/09/06 08:42:20 jdolecek Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
