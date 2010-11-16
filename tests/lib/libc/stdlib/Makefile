# $NetBSD: Makefile,v 1.4 2010/11/16 14:03:47 tron Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_mi_vector_hash t_environment t_environment_pth

LDADD.t_environment_pth=	-pthread

.include <bsd.test.mk>
