# $NetBSD: Makefile,v 1.1 2008/05/01 15:37:18 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-sh

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tests/atf/atf-sh

TESTS_SH=	h_misc
TESTS_SH+=	t_atf_check
TESTS_SH+=	t_config
TESTS_SH+=	t_normalize
TESTS_SH+=	t_tc
TESTS_SH+=	t_tp

.include <bsd.test.mk>
