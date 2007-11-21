# $NetBSD: Makefile,v 1.2 2007/11/21 15:39:33 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/sh_interface

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tests/atf/sh_interface

TESTS_SH=	h_misc
TESTS_SH+=	t_atf_check
TESTS_SH+=	t_config
TESTS_SH+=	t_normalize
TESTS_SH+=	t_tc
TESTS_SH+=	t_tp

.include <bsd.test.mk>
