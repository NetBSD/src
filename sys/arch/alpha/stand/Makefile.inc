# $NetBSD: Makefile.inc,v 1.22.106.1 2011/06/06 09:04:45 jruoho Exp $

NOMAN=			# defined

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC=		-static
