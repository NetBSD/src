# $NetBSD: Makefile,v 1.3 2011/05/05 05:24:45 jruoho Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

LDADD.t_bitops+=	-lm
TESTS_C=		t_bitops t_bootblock t_types

.include <bsd.test.mk>
