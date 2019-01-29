# $NetBSD: Makefile,v 1.5 2019/01/29 19:56:37 mgorny Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/usr.bin/cc

ASAN_TESTS=	#
ASAN_TESTS+=	t_asan_double_free
ASAN_TESTS+=	t_asan_global_buffer_overflow
ASAN_TESTS+=	t_asan_heap_overflow
ASAN_TESTS+=	t_asan_off_by_one
ASAN_TESTS+=	t_asan_poison
ASAN_TESTS+=	t_asan_uaf

TESTS_SH=	#
TESTS_SH+=	$(ASAN_TESTS)
TESTS_SH+=	t_ubsan_int_add_overflow
TESTS_SH+=	t_ubsan_int_sub_overflow
TESTS_SH+=	t_ubsan_int_neg_overflow
TESTS_SH+=	t_ubsan_int_divzero
TESTS_SH+=	t_ubsan_vla_out_of_bounds
TESTS_SH+=	t_hello

.for test in ${ASAN_TESTS}
TESTS_SH_SRC_${test}=	asan_common.subr ${test}.sh
.endfor

.include <bsd.test.mk>
