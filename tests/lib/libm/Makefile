# $NetBSD: Makefile,v 1.4 2011/04/08 06:49:21 jruoho Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libm

TESTS_C+=	t_ceil t_floor t_round t_tanh

LDADD+=-lm
COPTS+=		-Wfloat-equal

.include <bsd.test.mk>
