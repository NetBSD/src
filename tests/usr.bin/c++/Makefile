# $NetBSD: Makefile,v 1.7 2018/04/04 23:53:26 kamil Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/c++

TESTS_SH=	#
TESTS_SH+=	t_asan_double_free
TESTS_SH+=	t_asan_global_buffer_overflow
TESTS_SH+=	t_asan_heap_overflow
TESTS_SH+=	t_asan_off_by_one
TESTS_SH+=	t_asan_uaf
TESTS_SH+=	t_call_once
TESTS_SH+=	t_call_once2
TESTS_SH+=	t_cxxruntime
TESTS_SH+=	t_hello
TESTS_SH+=	t_pthread_once
TESTS_SH+=	t_static_destructor

.include <bsd.test.mk>
