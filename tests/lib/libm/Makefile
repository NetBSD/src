# $NetBSD: Makefile,v 1.6 2011/04/11 10:51:36 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libm

TESTS_C+=	t_ceil t_floor t_log t_round t_tanh t_infinity

LDADD+=		-lm
COPTS+=		-Wfloat-equal

.include <bsd.test.mk>
