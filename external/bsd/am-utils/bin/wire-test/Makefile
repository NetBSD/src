#	$NetBSD: Makefile,v 1.1.4.2 2008/10/19 22:39:28 haad Exp $

.include "${.CURDIR}/../Makefile.inc"

PROG=	wire-test

DIST=	${IDIST}/wire-test
.PATH:	${DIST}

SRCS=	wire-test.c
MAN+=	wire-test.8

LDADD+=	${LIBAMU}

.include <bsd.prog.mk>
