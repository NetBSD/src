# $NetBSD: Makefile,v 1.2 2020/06/29 08:55:00 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/sqlite

.PATH:		${SRCDIR}/utils/sqlite

TESTS_CXX=
.for test in	\
		c_gate_test \
		database_test \
		exceptions_test \
		statement_test \
		transaction_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
