# $NetBSD: Makefile,v 1.4 2011/05/05 13:36:05 jruoho Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

LDADD.t_bitops+=	-lm
TESTS_C=		t_bitops t_bootblock t_tree t_types

.include <bsd.test.mk>
