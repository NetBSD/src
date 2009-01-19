# $NetBSD: Makefile,v 1.1 2009/01/19 07:13:05 jmmv Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/atf/atf-c

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/tests/atf/atf-c

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${NETBSDSRCDIR}/external/bsd/atf/lib/libatf-c

PROGS=		h_check
SRCS.h_check=	h_check.c
MAN.h_check=	# defined
BINDIR.h_check=	${TESTSDIR}

TESTS_C=	t_check
TESTS_C+=	t_config
TESTS_C+=	t_dynstr
TESTS_C+=	t_env
TESTS_C+=	t_error
TESTS_C+=	t_expand
TESTS_C+=	t_fs
TESTS_C+=	t_io
TESTS_C+=	t_list
TESTS_C+=	t_macros
TESTS_C+=	t_map
TESTS_C+=	t_process
TESTS_C+=	t_sanity
TESTS_C+=	t_signals
TESTS_C+=	t_tc
TESTS_C+=	t_tcr
TESTS_C+=	t_text
TESTS_C+=	t_ui
TESTS_C+=	t_user

.include <bsd.test.mk>
