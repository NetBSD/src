# $NetBSD: Makefile.inc,v 1.2 2017/07/24 10:34:54 mrg Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../../..
LDSTATIC?=		-static
