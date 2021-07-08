# $NetBSD: Makefile,v 1.15 2021/07/08 09:07:46 christos Exp $

NOMAN=
.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/stdio

TESTS_C+=	t_clearerr
TESTS_C+=	t_fflush
TESTS_C+=	t_fmemopen
TESTS_C+=	t_fopen
TESTS_C+=	t_open_memstream
TESTS_C+=	t_fputc
TESTS_C+=	t_popen
TESTS_C+=	t_printf
TESTS_C+=	t_scanf

TESTS_SH+=	t_intr

COPTS.t_printf.c += -Wno-format-nonliteral

PROGS+=		h_intr h_makenumbers h_testnumbers

.include <bsd.test.mk>
