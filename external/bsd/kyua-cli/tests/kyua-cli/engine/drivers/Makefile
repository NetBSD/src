# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:58 lukem Exp $

KYUA_LIBS=	engine store engine utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/engine/drivers

.PATH:		${SRCDIR}/engine/drivers

TESTS_CXX=
.for test in	\
		debug_test_test \
		list_tests_test \
		run_tests_test \
		scan_action_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

PROGS_CXX=			list_tests_helpers
SRCS.list_tests_helpers=	list_tests_helpers.cpp
BINDIR.list_tests_helpers=	${TESTSDIR}
MAN.list_tests_helpers=		# none
LDADD.list_tests_helpers=	-latf-c++ -latf-c
DPADD.list_tests_helpers=	${LIBATF_CXX} ${LIBATF_C}

.include <bsd.test.mk>
