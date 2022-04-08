#	$NetBSD: Makefile,v 1.1 2022/04/08 23:35:52 riastradh Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libc/membar

TESTS_C+=	t_dekker
TESTS_C+=	t_seqlock
TESTS_C+=	t_spinlock

LDADD+=		-pthread

WARNS=		6

.include <bsd.test.mk>
