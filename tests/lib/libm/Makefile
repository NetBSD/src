# $NetBSD: Makefile,v 1.5 2011/04/10 06:11:47 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libm

TESTS_C+=	t_ceil t_floor t_log t_round t_tanh

LDADD+=		-lm
COPTS+=		-Wfloat-equal

.include <bsd.test.mk>
