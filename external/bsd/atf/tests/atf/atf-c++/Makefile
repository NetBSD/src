# $NetBSD: Makefile,v 1.7.2.1 2014/08/19 23:45:56 tls Exp $

.include <bsd.init.mk>

TESTS_SUBDIRS=	detail

TESTSDIR=	${TESTSBASE}/atf/atf-c++

.PATH:		${SRCDIR}/atf-c++
.PATH:		${SRCDIR}/atf-c++/detail

FILESDIR=	${TESTSDIR}
FILES=		macros_hpp_test.cpp
FILES+=		unused_test.cpp

TESTS_CXX=
.for test in	atf_c++_test \
		build_test \
		check_test \
		config_test \
		macros_test \
		tests_test \
		utils_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp test_helpers.cpp
.endfor
.undef test

TESTS_SH=	pkg_config_test

.include <bsd.test.mk>
