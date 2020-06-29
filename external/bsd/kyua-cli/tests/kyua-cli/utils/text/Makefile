# $NetBSD: Makefile,v 1.2 2020/06/29 08:55:00 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/text

.PATH:		${SRCDIR}/utils/text

TESTS_CXX=
.for test in	\
		exceptions_test \
		operations_test \
		table_test \
		templates_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
