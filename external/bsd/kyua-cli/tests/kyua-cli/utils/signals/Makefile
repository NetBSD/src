# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:33 yamt Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/signals

.PATH:		${SRCDIR}/utils/signals

TESTS_CXX=	exceptions_test \
		interrupts_test \
		misc_test \
		programmer_test

.include <bsd.test.mk>
