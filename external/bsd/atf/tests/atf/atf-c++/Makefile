# $NetBSD: Makefile,v 1.1 2009/01/19 07:13:05 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c++

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-c++

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libatf-c

TESTS_CXX=	t_check
TESTS_CXX+=	t_config
TESTS_CXX+=	t_env
TESTS_CXX+=	t_expand
TESTS_CXX+=	t_fs
TESTS_CXX+=	t_io
TESTS_CXX+=	t_macros
TESTS_CXX+=	t_parser
TESTS_CXX+=	t_process
TESTS_CXX+=	t_signals
TESTS_CXX+=	t_tests
TESTS_CXX+=	t_text
TESTS_CXX+=	t_user
TESTS_CXX+=	t_utils

.include <bsd.test.mk>
