# $NetBSD: Makefile,v 1.3 2014/02/08 19:15:34 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c++/detail
ATFFILE=	yes

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-c++/detail

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR:H}/lib/libatf-c  # For bconfig.h.

PROGS_CXX=
.for prog in	version_helper
PROGS_CXX+=	${prog}
SRCS.${prog}=	${prog}.cpp
MAN.${prog}=	# defined
BINDIR.${prog}=	${TESTSDIR}
.endfor

TESTS_CXX=
.for test in	application_test \
		auto_array_test \
		env_test \
		exceptions_test \
		fs_test \
		process_test \
		sanity_test \
		text_test
TESTS_CXX+=	${test}
SRCS.${test}=	${test}.cpp test_helpers.cpp
.endfor
.undef test

.include <bsd.test.mk>
