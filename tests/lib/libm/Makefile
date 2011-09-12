# $NetBSD: Makefile,v 1.8 2011/09/12 16:48:48 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libm

TESTS_C+=	t_ceil
TESTS_C+=	t_infinity
TESTS_C+=	t_ldexp
TESTS_C+=	t_log
TESTS_C+=	t_round
TESTS_C+=	t_scalbn
TESTS_C+=	t_tanh

LDADD+=		-lm
#COPTS+=	-Wfloat-equal

.include <bsd.test.mk>
