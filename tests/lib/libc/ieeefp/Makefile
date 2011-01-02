# $NetBSD: Makefile,v 1.2 2011/01/02 13:49:15 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/ieeefp

TESTS_C+=	t_except
TESTS_C+=	t_nan_inf
TESTS_C+=	t_round
TESTS_C+=	t_subnormal

LDADD.t_subnormal+=-lm
WARNS=1

.include <bsd.test.mk>

