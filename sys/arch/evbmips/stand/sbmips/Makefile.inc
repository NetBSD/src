# $NetBSD: Makefile.inc,v 1.3.4.2 2017/08/30 15:45:04 martin Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
