# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:33 yamt Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/config

.PATH:		${SRCDIR}/utils/config

TESTS_CXX=	exceptions_test \
		keys_test \
		lua_module_test \
		nodes_test \
		parser_test \
		tree_test

.include <bsd.test.mk>
