# $NetBSD: Makefile,v 1.4 2010/07/13 21:12:39 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-run
ATFFILE=	yes

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-run
.PATH:		${SRCDIR}/atf-c++
.PATH:		${.CURDIR}/../../../lib/libatf-c

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}/atf-run
CPPFLAGS+=	-I${SRCDIR}/atf-c++
CPPFLAGS+=	-I${.CURDIR}/../../../lib/libatf-c

TESTS_CXX=	atffile_test
TESTS_CXX+=	bad_metadata_helper
TESTS_CXX+=	config_test
TESTS_CXX+=	expect_helpers
TESTS_CXX+=	fs_test
TESTS_CXX+=	misc_helpers
TESTS_CXX+=	pass_helper
TESTS_CXX+=	requirements_test
TESTS_CXX+=	several_tcs_helper
TESTS_CXX+=	test_program_test
TESTS_CXX+=	zero_tcs_helper
TESTS_SH=	integration_test

SRCS.atffile_test=	atffile_test.cpp atffile.cpp test_helpers.cpp
SRCS.config_test=	config_test.cpp config.cpp test_helpers.cpp
SRCS.fs_test=		fs_test.cpp fs.cpp test_helpers.cpp
SRCS.requirements_test=	requirements_test.cpp requirements.cpp
SRCS.test_program_test=	test_program_test.cpp test-program.cpp fs.cpp timer.cpp

.include <bsd.test.mk>
