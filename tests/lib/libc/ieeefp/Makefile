# $NetBSD: Makefile,v 1.3 2011/01/02 21:17:19 christos Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/ieeefp

TESTS_C+=	t_except
TESTS_C+=	t_nan_inf
TESTS_C+=	t_round
TESTS_C+=	t_subnormal

LDADD.t_subnormal+=-lm

.include <bsd.test.mk>
