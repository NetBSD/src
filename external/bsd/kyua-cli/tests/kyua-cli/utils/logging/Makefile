# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:26 tls Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/logging

.PATH:		${SRCDIR}/utils/logging

TESTS_CXX=	macros_test \
		operations_test

.include <bsd.test.mk>
