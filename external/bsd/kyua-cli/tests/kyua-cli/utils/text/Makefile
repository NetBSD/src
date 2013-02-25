# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:27 tls Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/text

.PATH:		${SRCDIR}/utils/text

TESTS_CXX=	exceptions_test \
		operations_test \
		table_test \
		templates_test

.include <bsd.test.mk>
