# $NetBSD: Makefile,v 1.8 2019/08/18 20:15:58 kamil Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/cc

ASAN_TESTS=	#
ASAN_TESTS+=	t_asan_double_free
ASAN_TESTS+=	t_asan_global_buffer_overflow
ASAN_TESTS+=	t_asan_heap_overflow
ASAN_TESTS+=	t_asan_off_by_one
ASAN_TESTS+=	t_asan_poison
ASAN_TESTS+=	t_asan_uaf

UBSAN_TESTS=	#
UBSAN_TESTS+=	t_ubsan_int_add_overflow
UBSAN_TESTS+=	t_ubsan_int_neg_overflow
UBSAN_TESTS+=	t_ubsan_vla_out_of_bounds
UBSAN_TESTS+=	t_ubsan_int_sub_overflow
UBSAN_TESTS+=	t_ubsan_int_divzero

TESTS_SH=	#
TESTS_SH+=	$(ASAN_TESTS)
TESTS_SH+=	$(UBSAN_TESTS)
TESTS_SH+=	t_hello
TESTS_SH+=	t_libgomp

TESTS_SH+=	t_fuzzer_oom
TESTS_SH+=	t_fuzzer_simple
TESTS_SH+=	t_fuzzer_timeout

TESTS_SH+=	t_msan_allocated_memory
TESTS_SH+=	t_msan_check_mem
TESTS_SH+=	t_msan_free
TESTS_SH+=	t_msan_heap
TESTS_SH+=	t_msan_partial_poison
TESTS_SH+=	t_msan_poison
TESTS_SH+=	t_msan_realloc
TESTS_SH+=	t_msan_shadow
TESTS_SH+=	t_msan_stack
TESTS_SH+=	t_msan_unpoison

TESTS_SH+=	t_tsan_data_race
TESTS_SH+=	t_tsan_heap_use_after_free
TESTS_SH+=	t_tsan_lock_order_inversion
TESTS_SH+=	t_tsan_locked_mutex_destroy
TESTS_SH+=	t_tsan_signal_errno
TESTS_SH+=	t_tsan_thread_leak

.for test in ${ASAN_TESTS}
TESTS_SH_SRC_${test}=	asan_common.subr ${test}.sh
.endfor
.for test in ${UBSAN_TESTS}
TESTS_SH_SRC_${test}=	ubsan_common.subr ${test}.sh
.endfor

.include <bsd.test.mk>
