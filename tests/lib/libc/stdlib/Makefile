# $NetBSD: Makefile,v 1.3 2010/11/14 19:19:24 tron Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_mi_vector_hash t_environment

LDADD.t_environment=	-pthread

.include <bsd.test.mk>
