# $NetBSD: Makefile,v 1.1.4.2 2012/10/30 19:00:08 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/share/mk

TESTS_SH=	t_lib t_own t_prog t_test
.for test in ${TESTS_SH}
TESTS_SH_SRC_${test}=	${test}.sh common.subr
.endfor

.include <bsd.test.mk>
