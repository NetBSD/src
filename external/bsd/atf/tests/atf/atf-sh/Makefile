# $NetBSD: Makefile,v 1.2 2010/06/04 08:33:42 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-sh

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-sh

TESTS_SH=	h_misc
TESTS_SH+=	t_atf_check
TESTS_SH+=	t_config
TESTS_SH+=	t_integration
TESTS_SH+=	t_normalize
TESTS_SH+=	t_tc
TESTS_SH+=	t_tp

.include <bsd.test.mk>
