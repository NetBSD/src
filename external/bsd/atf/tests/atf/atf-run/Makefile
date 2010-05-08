# $NetBSD: Makefile,v 1.1 2010/05/08 08:12:34 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-run

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-run
.PATH:		${SRCDIR}/tests/atf/atf-run

CPPFLAGS+=	-I${SRCDIR}

TESTS_CXX=	h_bad_metadata
TESTS_CXX+=	h_fail
TESTS_CXX+=	h_misc
TESTS_CXX+=	h_pass
TESTS_CXX+=	h_several_tcs
TESTS_CXX+=	h_zero_tcs
TESTS_CXX+=	t_config
TESTS_CXX+=	t_requirements
TESTS_CXX+=	t_test_program
TESTS_SH=	t_integration

SRCS.t_config=		t_config.cpp config.cpp
SRCS.t_requirements=	t_requirements.cpp requirements.cpp
SRCS.t_test_program=	t_test_program.cpp test-program.cpp timer.cpp

.include <bsd.test.mk>
