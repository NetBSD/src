# $NetBSD: Makefile,v 1.5.6.1 2014/05/22 15:40:15 yamt Exp $

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-sh
ATFFILE=	yes
KYUAFILE=	yes

.PATH:		${SRCDIR}/atf-sh

TESTS_SH=	atf-check_test
TESTS_SH+=	atf_check_test
TESTS_SH+=	config_test
TESTS_SH+=	integration_test
TESTS_SH+=	misc_helpers
TESTS_SH+=	normalize_test
TESTS_SH+=	tc_test
TESTS_SH+=	tp_test

.include <bsd.test.mk>
