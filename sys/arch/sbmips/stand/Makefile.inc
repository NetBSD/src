# $NetBSD: Makefile.inc,v 1.1.2.2 2002/11/11 22:03:22 nathanw Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
