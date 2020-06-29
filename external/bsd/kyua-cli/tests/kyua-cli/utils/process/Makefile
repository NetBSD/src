# $NetBSD: Makefile,v 1.2 2020/06/29 08:55:00 lukem Exp $

KYUA_LIBS=	utils

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/kyua-cli/utils/process

.PATH:		${SRCDIR}/utils/process

TESTS_CXX=
.for test in	\
		child_test \
		exceptions_test \
		fdstream_test \
		status_test \
		systembuf_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp
.endfor

PROGS_CXX=	helpers
SRCS.helpers=	helpers.cpp
BINDIR.helpers=	${TESTSDIR}
MAN.helpers=	# none

.include <bsd.test.mk>
