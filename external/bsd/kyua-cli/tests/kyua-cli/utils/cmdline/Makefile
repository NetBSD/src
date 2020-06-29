# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:59 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/cmdline

.PATH:		${SRCDIR}/utils/cmdline

TESTS_CXX=
.for test in	\
		base_command_test \
		commands_map_test \
		exceptions_test \
		globals_test \
		options_test \
		parser_test \
		ui_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
