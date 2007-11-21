# $NetBSD: Makefile,v 1.2 2007/11/21 15:39:34 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/tools

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tests/atf/tools

TESTS_CXX=	h_fail
TESTS_CXX+=	h_misc
TESTS_CXX+=	h_pass
TESTS_SH=	t_atf_cleanup
TESTS_SH+=	t_atf_compile
TESTS_SH+=	t_atf_config
TESTS_SH+=	t_atf_report
TESTS_SH+=	t_atf_run

BINDIR=		${TESTSDIR}
PROGS_CXX=	h_mode
MAN.h_mode=	# empty

.include <bsd.test.mk>
