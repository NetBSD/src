#	$NetBSD: Makefile,v 1.1.1.2 2000/11/20 00:16:18 wiz Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	wire-test

DIST=	${IDIST}/wire-test
.PATH:	${DIST}

SRCS=	wire-test.c
MAN+=	wire-test.8

LDADD+=	${LIBAMU}

.include <bsd.prog.mk>
