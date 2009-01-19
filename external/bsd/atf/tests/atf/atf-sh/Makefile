# $NetBSD: Makefile,v 1.1 2009/01/19 07:13:05 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-sh

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-sh

TESTS_SH=	h_misc
TESTS_SH+=	t_atf_check
TESTS_SH+=	t_config
TESTS_SH+=	t_normalize
TESTS_SH+=	t_tc
TESTS_SH+=	t_tp

.include <bsd.test.mk>
