# $NetBSD: Makefile,v 1.2.4.2 2008/01/09 01:59:05 matt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/units

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tests/atf/units

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libatfprivate # for config.h
CPPFLAGS+=	-I${SRCDIR}/libs

TESTS_CXX=	t_config
TESTS_CXX+=	t_env
TESTS_CXX+=	t_expand
TESTS_CXX+=	t_fs
TESTS_CXX+=	t_io
TESTS_CXX+=	t_parser
TESTS_CXX+=	t_sanity
TESTS_CXX+=	t_tests
TESTS_CXX+=	t_text
TESTS_CXX+=	t_user
TESTS_CXX+=	t_utils

.include <bsd.test.mk>
