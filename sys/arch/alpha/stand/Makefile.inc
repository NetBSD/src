# $NetBSD: Makefile.inc,v 1.16 1999/04/11 02:51:21 cgd Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
