# $NetBSD: Makefile.inc,v 1.15.2.2 1999/04/05 03:42:51 cgd Exp $

.include <bsd.own.mk>

BINDIR=			/usr/mdec
WARNS?=			1

MKMAN=			no

CPPFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
