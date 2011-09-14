# $NetBSD: Makefile,v 1.9 2011/09/14 05:18:19 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libm

TESTS_C+=	t_ceil
TESTS_C+=	t_cos
TESTS_C+=	t_infinity
TESTS_C+=	t_ldexp
TESTS_C+=	t_log
TESTS_C+=	t_round
TESTS_C+=	t_scalbn
TESTS_C+=	t_sin
TESTS_C+=	t_tan
TESTS_C+=	t_tanh

LDADD+=		-lm
#COPTS+=	-Wfloat-equal

.include <bsd.test.mk>
