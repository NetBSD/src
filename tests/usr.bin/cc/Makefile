# $NetBSD: Makefile,v 1.1.32.1 2018/04/07 04:12:21 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/cc

TESTS_SH=	#
TESTS_SH+=	t_asan_double_free
TESTS_SH+=	t_asan_global_buffer_overflow
TESTS_SH+=	t_asan_heap_overflow
TESTS_SH+=	t_asan_off_by_one
TESTS_SH+=	t_asan_uaf
TESTS_SH+=	t_hello

.include <bsd.test.mk>
