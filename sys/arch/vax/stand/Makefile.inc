# $NetBSD: Makefile.inc,v 1.1.4.2 2000/06/22 17:05:12 minoura Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
