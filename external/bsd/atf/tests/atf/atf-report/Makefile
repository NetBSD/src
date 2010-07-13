# $NetBSD: Makefile,v 1.4 2010/07/13 21:12:39 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-report
ATFFILE=	yes

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-report
.PATH:		${SRCDIR}/atf-c++

TESTS_CXX=	fail_helper
TESTS_CXX+=	misc_helpers
TESTS_CXX+=	pass_helper
TESTS_CXX+=	reader_test
TESTS_SH=	integration_test

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${SRCDIR}/atf-c++

SRCS.reader_test=	reader_test.cpp reader.cpp

.include <bsd.test.mk>
