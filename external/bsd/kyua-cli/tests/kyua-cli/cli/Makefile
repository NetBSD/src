# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:58 lukem Exp $

KYUA_LIBS=	cli engine store engine utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/cli

.PATH:		${SRCDIR}/cli

TESTS_CXX=
.for test in	\
		cmd_about_test \
		cmd_config_test \
		cmd_db_exec_test \
		cmd_db_migrate_test \
		cmd_debug_test \
		cmd_help_test \
		cmd_list_test \
		cmd_report_html_test \
		cmd_report_test \
                cmd_test_test \
		common_test \
		config_test \
		main_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

.include <bsd.test.mk>
