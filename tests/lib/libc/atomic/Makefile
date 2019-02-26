# $NetBSD: Makefile,v 1.2 2019/02/26 10:01:41 isaki Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/atomic

TESTS_C+=	t_atomic_add
TESTS_C+=	t_atomic_and
TESTS_C+=	t_atomic_cas
TESTS_C+=	t_atomic_dec
TESTS_C+=	t_atomic_inc
TESTS_C+=	t_atomic_or
TESTS_C+=	t_atomic_swap

TESTS_C+=	t___sync_add
TESTS_C+=	t___sync_sub
TESTS_C+=	t___sync_or
TESTS_C+=	t___sync_and
TESTS_C+=	t___sync_xor
TESTS_C+=	t___sync_nand
TESTS_C+=	t___sync_compare_and_swap
TESTS_C+=	t___sync_lock

# The code conforms to new NAND semantics.  So this warning is not
# necessary here.
.if "${ACTIVE_CC}" == "gcc"
CPPFLAGS.t___sync_nand.c+=	-Wno-sync-nand
.elif "${ACTIVE_CC}" == "clang"
CPPFLAGS.t___sync_nand.c+=	-Wno-sync-fetch-and-nand-semantics-changed
.endif

MKMAN=no

BINDIR=		${TESTSDIR}

.include <bsd.test.mk>
