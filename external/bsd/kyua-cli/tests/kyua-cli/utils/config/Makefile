# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:59 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/config

.PATH:		${SRCDIR}/utils/config

TESTS_CXX=
.for test in	\
		exceptions_test \
		keys_test \
		lua_module_test \
		nodes_test \
		parser_test \
		tree_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
