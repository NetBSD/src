# $NetBSD: Makefile,v 1.4 2010/07/03 08:14:12 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c++

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-c++

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libatf-c

FILESDIR=	${TESTSDIR}
FILES=		macros_hpp_test.cpp

TESTS_CXX=
.for test in	application_test \
		atf_c++_test \
		build_test \
		check_test \
		config_test \
		env_test \
		exceptions_test \
		expand_test \
		fs_test \
		io_test \
		macros_test \
		parser_test \
		process_test \
		sanity_test \
		signals_test \
		tests_test \
		text_test \
		ui_test \
		user_test \
		utils_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp test_helpers.cpp
.endfor
.undef test

TESTS_SH=	pkg_config_test

.include <bsd.test.mk>
