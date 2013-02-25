# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:26 tls Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/fs

.PATH:		${SRCDIR}/utils/fs

TESTS_CXX=	auto_cleaners_test \
		exceptions_test \
		lua_module_test \
		operations_test \
		path_test

.include <bsd.test.mk>
