# $NetBSD: Makefile,v 1.6 2010/12/23 15:27:44 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdlib

TESTS_C+=	t_div t_mi_vector_hash t_environment t_environment_pth
TESTS_C+=	t_strtox

LDADD.t_environment_pth=	-pthread

.include <bsd.test.mk>
