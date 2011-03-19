# $NetBSD: Makefile,v 1.2 2011/03/19 06:39:17 jruoho Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=		${TESTSBASE}/include/sys

LDADD.t_bitops+=	-lm
TESTS_C=		t_bitops t_bootblock

.include <bsd.test.mk>
