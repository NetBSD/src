# $NetBSD: Makefile.inc,v 1.3.4.3 2017/08/31 08:18:08 martin Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec/sbmips
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../../..
LDSTATIC?=		-static
