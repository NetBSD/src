# $NetBSD: Makefile.inc,v 1.1 2002/11/09 06:20:37 cgd Exp $

.include <bsd.own.mk>

BINDIR?=		/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
