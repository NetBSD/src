# $NetBSD: Makefile.inc,v 1.14 1999/04/05 03:35:24 cgd Exp $

.include <bsd.own.mk>

BINDIR=			/usr/mdec
WARNS?=			1

MKMAN=			no

CFLAGS+=		-I${.CURDIR}/../..
LDSTATIC?=		-static
