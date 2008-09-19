#	$NetBSD: Makefile,v 1.1 2008/09/19 21:38:41 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	wire-test

DIST=	${IDIST}/wire-test
.PATH:	${DIST}

SRCS=	wire-test.c
MAN+=	wire-test.8

LDADD+=	${LIBAMU}

.include <bsd.prog.mk>
