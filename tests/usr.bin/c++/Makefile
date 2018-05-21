# $NetBSD: Makefile,v 1.3.10.4 2018/05/21 04:36:18 pgoyette Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/c++

TESTS_SH=	#
TESTS_SH+=	t_asan_double_free
TESTS_SH+=	t_asan_global_buffer_overflow
TESTS_SH+=	t_asan_heap_overflow
TESTS_SH+=	t_asan_off_by_one
TESTS_SH+=	t_asan_poison
TESTS_SH+=	t_asan_uaf
TESTS_SH+=	t_ubsan_int_add_overflow
TESTS_SH+=	t_ubsan_int_sub_overflow
TESTS_SH+=	t_ubsan_int_neg_overflow
TESTS_SH+=	t_ubsan_int_divzero
TESTS_SH+=	t_ubsan_vla_out_of_bounds
TESTS_SH+=	t_call_once
TESTS_SH+=	t_call_once2
TESTS_SH+=	t_cxxruntime
TESTS_SH+=	t_hello
TESTS_SH+=	t_pthread_once
TESTS_SH+=	t_static_destructor

.include <bsd.test.mk>
