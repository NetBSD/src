# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:26 tls Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/signals

.PATH:		${SRCDIR}/utils/signals

TESTS_CXX=	exceptions_test \
		interrupts_test \
		misc_test \
		programmer_test

.include <bsd.test.mk>
