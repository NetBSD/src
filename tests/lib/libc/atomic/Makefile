# $NetBSD: Makefile,v 1.1 2019/02/17 12:24:17 isaki Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/atomic

TESTS_C+=	t_atomic_add
TESTS_C+=	t_atomic_and
TESTS_C+=	t_atomic_cas
TESTS_C+=	t_atomic_dec
TESTS_C+=	t_atomic_inc
TESTS_C+=	t_atomic_or
TESTS_C+=	t_atomic_swap

MKMAN=no

BINDIR=		${TESTSDIR}

.include <bsd.test.mk>
