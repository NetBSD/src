# $NetBSD: Makefile.inc,v 1.1.1.1 2017/07/24 08:56:29 mrg Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
