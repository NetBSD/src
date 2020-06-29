# $NetBSD: Makefile,v 1.2 2020/06/29 08:55:00 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/signals

.PATH:		${SRCDIR}/utils/signals

TESTS_CXX=
.for test in	\
		exceptions_test \
		interrupts_test \
		misc_test \
		programmer_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
