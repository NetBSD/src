# $NetBSD: Makefile.inc,v 1.3 2017/08/15 10:11:31 mrg Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec/sbmips
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../../..
LDSTATIC?=		-static
