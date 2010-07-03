# $NetBSD: Makefile,v 1.3 2010/07/03 08:14:12 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-sh

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/atf-sh

CPPFLAGS+=	-DHAVE_CONFIG_H

TESTS_SH=	atf_check_test
TESTS_SH+=	config_test
TESTS_SH+=	integration_test
TESTS_SH+=	misc_helpers
TESTS_SH+=	normalize_test
TESTS_SH+=	tc_test
TESTS_SH+=	tp_test

.include <bsd.test.mk>
