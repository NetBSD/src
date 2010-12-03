# $NetBSD: Makefile,v 1.5 2010/12/03 13:11:50 njoly Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_mi_vector_hash t_environment t_environment_pth
TESTS_C+=	t_strtox

LDADD.t_environment_pth=	-pthread

.include <bsd.test.mk>
