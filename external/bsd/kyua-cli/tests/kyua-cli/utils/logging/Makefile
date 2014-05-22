# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:33 yamt Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/logging

.PATH:		${SRCDIR}/utils/logging

TESTS_CXX=	macros_test \
		operations_test

.include <bsd.test.mk>
