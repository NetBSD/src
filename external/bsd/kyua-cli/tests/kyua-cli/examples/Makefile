# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:59 lukem Exp $

KYUA_LIBS=	engine utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/examples

.PATH:		${SRCDIR}/examples

TESTS_CXX=
.for test in	\
		syntax_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
