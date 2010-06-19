# $NetBSD: Makefile,v 1.2 2010/06/19 10:20:21 pooka Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-report

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-report

TESTS_CXX=	h_fail
TESTS_CXX+=	h_misc
TESTS_CXX+=	h_pass
TESTS_CXX+=	h_xfail
TESTS_SH=	t_integration

.include <bsd.test.mk>
