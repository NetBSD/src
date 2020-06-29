# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:59 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/format

.PATH:		${SRCDIR}/utils/format

TESTS_CXX=
.for test in	\
		exceptions_test \
		formatter_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
