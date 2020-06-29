# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:58 lukem Exp $

KYUA_LIBS=	engine store engine utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/engine

.PATH:		${SRCDIR}/engine

TESTS_CXX=
.for test in	\
		action_test \
		config_test \
		context_test \
		exceptions_test \
		filters_test \
		kyuafile_test \
		metadata_test \
		test_case_test \
		test_program_test \
                test_result_test \
		testers_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

PROGS_CXX=			test_case_atf_helpers
SRCS.test_case_atf_helpers=	test_case_atf_helpers.cpp
BINDIR.test_case_atf_helpers=	${TESTSDIR}
MAN.test_case_atf_helpers=	# none
LDADD.test_case_atf_helpers=	-latf-c++ -latf-c
DPADD.test_case_atf_helpers=	${LIBATF_CXX} ${LIBATF_C}

PROGS_CXX+=			test_case_plain_helpers
SRCS.test_case_plain_helpers=	test_case_plain_helpers.cpp
BINDIR.test_case_plain_helpers=	${TESTSDIR}
MAN.test_case_plain_helpers=	# none

TESTS_SUBDIRS+=	drivers

.include <bsd.test.mk>
