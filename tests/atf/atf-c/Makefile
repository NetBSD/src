# $NetBSD: Makefile,v 1.1.4.2 2008/05/18 12:36:00 yamt Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/tests/atf/atf-c

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/lib/libatf-c

TESTS_C=	t_config
TESTS_C+=	t_dynstr
TESTS_C+=	t_env
TESTS_C+=	t_error
TESTS_C+=	t_expand
TESTS_C+=	t_fs
TESTS_C+=	t_io
TESTS_C+=	t_list
TESTS_C+=	t_macros
TESTS_C+=	t_map
TESTS_C+=	t_sanity
TESTS_C+=	t_signals
TESTS_C+=	t_tc
TESTS_C+=	t_tcr
TESTS_C+=	t_text
TESTS_C+=	t_ui
TESTS_C+=	t_user

.include <bsd.test.mk>
