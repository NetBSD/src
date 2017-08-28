# $NetBSD: Makefile.inc,v 1.3.2.2 2017/08/28 17:51:36 skrll Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec/sbmips
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../../..
LDSTATIC?=		-static
