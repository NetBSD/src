# $NetBSD: Makefile,v 1.2 2020/06/29 08:54:59 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils

.PATH:		${SRCDIR}/utils

TESTS_CXX=
.for test in	\
		auto_array_test \
		datetime_test \
		env_test \
		memory_test \
		optional_test \
		passwd_test \
		sanity_test \
		stream_test \
		units_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

TESTS_SUBDIRS+=	cmdline config format fs logging process signals sqlite text

.include <bsd.test.mk>
